// Nymoris Local LLM Inference Engine
// Minimal transformer inference in C with no libc dependency.
// Supports small GPT-style models in a custom flat binary format.

#include "llm.h"

typedef unsigned long size_t;

// Syscalls provided by init.c
extern int sys_open(const char *path, int flags, int mode);
extern void sys_close(int fd);
extern int sys_read(int fd, char *buf, size_t len);

// ============================================================================
// Math
// ============================================================================

static float rsqrt(float x) {
    int i = *(int*)&x;
    i = 0x5f3759df - (i >> 1);
    float y = *(float*)&i;
    y = y * (1.5f - 0.5f * x * y * y);
    return y;
}

static float sqrtf(float x) {
    return x * rsqrt(x);
}

static float expf(float x) {
    if (x < -88.0f) return 0.0f;
    if (x > 88.0f) return 1e38f;
    float k = (int)(x * 1.442695f);
    float r = x - k * 0.693147f;
    float r2 = r * r;
    float y = 1.0f + r + r2 * 0.5f + r2 * r * 0.166667f + r2 * r2 * 0.041667f;
    int iy = *(int*)&y;
    iy += ((int)k) << 23;
    return *(float*)&iy;
}

static float silu(float x) {
    return x / (1.0f + expf(-x));
}

static void softmax(float *x, int n) {
    float max_val = x[0];
    for (int i = 1; i < n; i++) if (x[i] > max_val) max_val = x[i];
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    float inv_sum = 1.0f / sum;
    for (int i = 0; i < n; i++) x[i] *= inv_sum;
}

static void layer_norm(float *x, int n, float *weight, float *bias) {
    float mean = 0.0f;
    for (int i = 0; i < n; i++) mean += x[i];
    mean /= n;
    float var = 0.0f;
    for (int i = 0; i < n; i++) {
        float d = x[i] - mean;
        var += d * d;
    }
    var /= n;
    float scale = rsqrt(var + 1e-5f);
    for (int i = 0; i < n; i++) {
        x[i] = (x[i] - mean) * scale * weight[i] + bias[i];
    }
}

static void rms_norm(float *x, int n, float *weight) {
    float ss = 0.0f;
    for (int i = 0; i < n; i++) ss += x[i] * x[i];
    ss /= n;
    float scale = rsqrt(ss + 1e-5f);
    for (int i = 0; i < n; i++) x[i] = x[i] * scale * weight[i];
}

static void matmul(float *out, const float *x, const float *w, int n, int d) {
    // out[d] = x[n] @ w[n,d]
    for (int i = 0; i < d; i++) {
        float sum = 0.0f;
        for (int j = 0; j < n; j++) sum += x[j] * w[j * d + i];
        out[i] = sum;
    }
}

static void matmul_add_bias(float *out, const float *x, const float *w, const float *b, int n, int d) {
    for (int i = 0; i < d; i++) {
        float sum = b[i];
        for (int j = 0; j < n; j++) sum += x[j] * w[j * d + i];
        out[i] = sum;
    }
}

// ============================================================================
// Arena allocator
// ============================================================================

static char llm_arena[200 * 1024 * 1024]; // 200MB
static int llm_arena_pos = 0;

static void* llm_alloc(int size) {
    // Align to 64 bytes
    while (llm_arena_pos % 64 != 0) llm_arena_pos++;
    if (llm_arena_pos + size > (int)sizeof(llm_arena)) return 0;
    void *p = &llm_arena[llm_arena_pos];
    llm_arena_pos += size;
    return p;
}

static void llm_reset() {
    llm_arena_pos = 0;
}

// ============================================================================
// Model format loader
// ============================================================================

// Simple flat binary format:
// Header: "NYMOLLM\0" (8 bytes)
// uint32: version (1)
// uint32: n_vocab
// uint32: n_ctx
// uint32: n_embd
// uint32: n_head
// uint32: n_layer
// uint32: n_ff
// uint32: weight_dtype (0=f32, 1=f16)
// Then tensors in order:
//   tok_emb [n_vocab, n_embd]
//   pos_emb [n_ctx, n_embd] (optional, may be zeros if RoPE)
//   For each layer:
//     attn_norm_w [n_embd]
//     attn_norm_b [n_embd] (may be zeros)
//     q_w [n_embd, n_embd], q_b [n_embd]
//     k_w [n_embd, n_embd], k_b [n_embd]
//     v_w [n_embd, n_embd], v_b [n_embd]
//     o_w [n_embd, n_embd], o_b [n_embd]
//     ffn_norm_w [n_embd]
//     ffn_norm_b [n_embd]
//     ffn_up_w [n_embd, n_ff], ffn_up_b [n_ff]
//     ffn_down_w [n_ff, n_embd], ffn_down_b [n_embd]
//   final_norm_w [n_embd]
//   final_norm_b [n_embd]
//   lm_head_w [n_embd, n_vocab] (often shared with tok_emb)

static int read_file(const char *path, char *buf, int max) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) return -1;
    int total = 0;
    int n;
    while ((n = sys_read(fd, buf + total, max - total)) > 0 && total < max) total += n;
    sys_close(fd);
    return total;
}

typedef struct {
    int n_vocab, n_ctx, n_embd, n_head, n_layer, n_ff;
    int dtype; // 0=f32

    float *tok_emb;
    float *pos_emb;

    float **attn_norm_w, **attn_norm_b;
    float **q_w, **q_b, **k_w, **k_b, **v_w, **v_b, **o_w, **o_b;
    float **ffn_norm_w, **ffn_norm_b;
    float **ffn_up_w, **ffn_up_b, **ffn_down_w, **ffn_down_b;

    float *final_norm_w, *final_norm_b;
    float *lm_head_w;

    // KV cache
    float *key_cache; // [n_layer, n_ctx, n_embd]
    float *value_cache; // [n_layer, n_ctx, n_embd]
} Model;

static int load_model(const char *path, Model *m) {
    char header[64];
    int fd = sys_open(path, 0, 0);
    if (fd < 0) return -1;

    // Read header
    int n = sys_read(fd, header, 64);
    if (n < 64) { sys_close(fd); return -1; }

    // Check magic
    if (header[0] != 'N' || header[1] != 'Y' || header[2] != 'M' || header[3] != 'O' ||
        header[4] != 'L' || header[5] != 'L' || header[6] != 'M' || header[7] != 0) {
        sys_close(fd); return -1;
    }

    int *h = (int*)(header + 8);
    int version = h[0];
    if (version != 1) { sys_close(fd); return -1; }

    m->n_vocab = h[1];
    m->n_ctx   = h[2];
    m->n_embd  = h[3];
    m->n_head  = h[4];
    m->n_layer = h[5];
    m->n_ff    = h[6];
    m->dtype   = h[7];

    if (m->dtype != 0) { sys_close(fd); return -1; } // Only f32 for now

    int nv = m->n_vocab;
    int nc = m->n_ctx;
    int ne = m->n_embd;
    int nh = m->n_head;
    int nl = m->n_layer;
    int nf = m->n_ff;
    int hs = ne / nh; // head size

    // Allocate tensors
    m->tok_emb = llm_alloc(nv * ne * sizeof(float));
    m->pos_emb = llm_alloc(nc * ne * sizeof(float));

    m->attn_norm_w = llm_alloc(nl * sizeof(float*));
    m->attn_norm_b = llm_alloc(nl * sizeof(float*));
    m->q_w = llm_alloc(nl * sizeof(float*));
    m->q_b = llm_alloc(nl * sizeof(float*));
    m->k_w = llm_alloc(nl * sizeof(float*));
    m->k_b = llm_alloc(nl * sizeof(float*));
    m->v_w = llm_alloc(nl * sizeof(float*));
    m->v_b = llm_alloc(nl * sizeof(float*));
    m->o_w = llm_alloc(nl * sizeof(float*));
    m->o_b = llm_alloc(nl * sizeof(float*));
    m->ffn_norm_w = llm_alloc(nl * sizeof(float*));
    m->ffn_norm_b = llm_alloc(nl * sizeof(float*));
    m->ffn_up_w = llm_alloc(nl * sizeof(float*));
    m->ffn_up_b = llm_alloc(nl * sizeof(float*));
    m->ffn_down_w = llm_alloc(nl * sizeof(float*));
    m->ffn_down_b = llm_alloc(nl * sizeof(float*));

    for (int l = 0; l < nl; l++) {
        m->attn_norm_w[l] = llm_alloc(ne * sizeof(float));
        m->attn_norm_b[l] = llm_alloc(ne * sizeof(float));
        m->q_w[l] = llm_alloc(ne * ne * sizeof(float));
        m->q_b[l] = llm_alloc(ne * sizeof(float));
        m->k_w[l] = llm_alloc(ne * ne * sizeof(float));
        m->k_b[l] = llm_alloc(ne * sizeof(float));
        m->v_w[l] = llm_alloc(ne * ne * sizeof(float));
        m->v_b[l] = llm_alloc(ne * sizeof(float));
        m->o_w[l] = llm_alloc(ne * ne * sizeof(float));
        m->o_b[l] = llm_alloc(ne * sizeof(float));
        m->ffn_norm_w[l] = llm_alloc(ne * sizeof(float));
        m->ffn_norm_b[l] = llm_alloc(ne * sizeof(float));
        m->ffn_up_w[l] = llm_alloc(ne * nf * sizeof(float));
        m->ffn_up_b[l] = llm_alloc(nf * sizeof(float));
        m->ffn_down_w[l] = llm_alloc(nf * ne * sizeof(float));
        m->ffn_down_b[l] = llm_alloc(ne * sizeof(float));
    }

    m->final_norm_w = llm_alloc(ne * sizeof(float));
    m->final_norm_b = llm_alloc(ne * sizeof(float));
    m->lm_head_w = llm_alloc(ne * nv * sizeof(float));

    // KV cache
    m->key_cache = llm_alloc(nl * nc * ne * sizeof(float));
    m->value_cache = llm_alloc(nl * nc * ne * sizeof(float));

    // Read weights from file
    char *wptr = (char*)m->tok_emb;
    int wsize = llm_arena_pos - ((char*)m->tok_emb - llm_arena);
    // Read rest of file directly into arena
    int read_total = 0;
    while ((n = sys_read(fd, wptr + read_total, wsize - read_total)) > 0) read_total += n;
    sys_close(fd);

    // Zero KV cache
    for (int i = 0; i < nl * nc * ne; i++) m->key_cache[i] = 0.0f;
    for (int i = 0; i < nl * nc * ne; i++) m->value_cache[i] = 0.0f;

    return 0;
}

// ============================================================================
// Tokenizer
// ============================================================================

// Simple vocabulary: each token is a byte sequence.
// Vocab file format:
//   First 4 bytes: number of tokens (int32)
//   Then for each token: 1 byte length, then bytes

typedef struct {
    int n_tokens;
    unsigned char **tokens;
    int *token_lens;
} Tokenizer;

static int load_tokenizer(const char *path, Tokenizer *t) {
    char buf[8 * 1024 * 1024]; // 8MB vocab buffer
    int size = read_file(path, buf, sizeof(buf));
    if (size < 4) return -1;

    t->n_tokens = *(int*)buf;
    if (t->n_tokens <= 0 || t->n_tokens > 100000) return -1;

    t->tokens = llm_alloc(t->n_tokens * sizeof(unsigned char*));
    t->token_lens = llm_alloc(t->n_tokens * sizeof(int));

    int pos = 4;
    for (int i = 0; i < t->n_tokens; i++) {
        if (pos >= size) return -1;
        int len = (unsigned char)buf[pos++];
        t->token_lens[i] = len;
        t->tokens[i] = llm_alloc(len);
        for (int j = 0; j < len && pos < size; j++) {
            t->tokens[i][j] = buf[pos++];
        }
    }
    return 0;
}

static int encode(const Tokenizer *t, const char *text, int *tokens, int max_tokens) {
    // Greedy longest-match tokenization
    int n = 0;
    int text_len = 0;
    while (text[text_len]) text_len++;

    int i = 0;
    while (i < text_len && n < max_tokens) {
        int best_len = 0;
        int best_id = 0;
        for (int id = 0; id < t->n_tokens; id++) {
            int len = t->token_lens[id];
            if (len > best_len && i + len <= text_len) {
                int match = 1;
                for (int j = 0; j < len; j++) {
                    if ((unsigned char)text[i + j] != t->tokens[id][j]) { match = 0; break; }
                }
                if (match) { best_len = len; best_id = id; }
            }
        }
        if (best_len == 0) {
            // Unknown byte, map to byte-fallback token if available
            best_len = 1;
            best_id = (unsigned char)text[i];
            if (best_id >= t->n_tokens) best_id = 0;
        }
        tokens[n++] = best_id;
        i += best_len;
    }
    return n;
}

static void decode(const Tokenizer *t, int *tokens, int n, char *out, int max_out) {
    int pos = 0;
    for (int i = 0; i < n && pos < max_out - 1; i++) {
        int id = tokens[i];
        if (id < 0 || id >= t->n_tokens) id = 0;
        for (int j = 0; j < t->token_lens[id] && pos < max_out - 1; j++) {
            out[pos++] = t->tokens[id][j];
        }
    }
    out[pos] = '\0';
}

// ============================================================================
// Transformer forward pass
// ============================================================================

static void transformer(int token, int pos, Model *m, float *out) {
    int ne = m->n_embd;
    int nh = m->n_head;
    int nl = m->n_layer;
    int nf = m->n_ff;
    int hs = ne / nh;
    int nc = m->n_ctx;

    // Copy token embedding
    float *x = llm_alloc(ne * sizeof(float));
    for (int i = 0; i < ne; i++) x[i] = m->tok_emb[token * ne + i];

    // Add position embedding
    for (int i = 0; i < ne; i++) x[i] += m->pos_emb[pos * ne + i];

    // Forward through layers
    for (int l = 0; l < nl; l++) {
        // Attention
        float *attn_out = llm_alloc(ne * sizeof(float));
        {
            // Pre-norm
            for (int i = 0; i < ne; i++) attn_out[i] = x[i];
            layer_norm(attn_out, ne, m->attn_norm_w[l], m->attn_norm_b[l]);

            // QKV projections
            float *q = llm_alloc(ne * sizeof(float));
            float *k = llm_alloc(ne * sizeof(float));
            float *v = llm_alloc(ne * sizeof(float));
            matmul_add_bias(q, attn_out, m->q_w[l], m->q_b[l], ne, ne);
            matmul_add_bias(k, attn_out, m->k_w[l], m->k_b[l], ne, ne);
            matmul_add_bias(v, attn_out, m->v_w[l], m->v_b[l], ne, ne);

            // Store k, v in cache
            for (int i = 0; i < ne; i++) {
                m->key_cache[l * nc * ne + pos * ne + i] = k[i];
                m->value_cache[l * nc * ne + pos * ne + i] = v[i];
            }

            // Multi-head attention
            float *attn_scores = llm_alloc(nh * nc * sizeof(float));
            for (int h = 0; h < nh; h++) {
                for (int t = 0; t <= pos; t++) {
                    float score = 0.0f;
                    for (int i = 0; i < hs; i++) {
                        score += q[h * hs + i] * m->key_cache[l * nc * ne + t * ne + h * hs + i];
                    }
                    score /= sqrtf((float)hs);
                    attn_scores[h * nc + t] = score;
                }
                // Softmax over valid positions
                softmax(&attn_scores[h * nc], pos + 1);
            }

            // Weighted sum of values
            for (int i = 0; i < ne; i++) attn_out[i] = 0.0f;
            for (int h = 0; h < nh; h++) {
                for (int t = 0; t <= pos; t++) {
                    float a = attn_scores[h * nc + t];
                    for (int i = 0; i < hs; i++) {
                        attn_out[h * hs + i] += a * m->value_cache[l * nc * ne + t * ne + h * hs + i];
                    }
                }
            }

            // Output projection
            float *proj_out = llm_alloc(ne * sizeof(float));
            matmul_add_bias(proj_out, attn_out, m->o_w[l], m->o_b[l], ne, ne);

            // Residual
            for (int i = 0; i < ne; i++) x[i] += proj_out[i];
        }

        // FFN
        float *ffn_out = llm_alloc(ne * sizeof(float));
        {
            // Pre-norm
            for (int i = 0; i < ne; i++) ffn_out[i] = x[i];
            layer_norm(ffn_out, ne, m->ffn_norm_w[l], m->ffn_norm_b[l]);

            // Up-project
            float *up = llm_alloc(nf * sizeof(float));
            matmul_add_bias(up, ffn_out, m->ffn_up_w[l], m->ffn_up_b[l], ne, nf);

            // GELU activation (approximate with SiLU for simplicity)
            for (int i = 0; i < nf; i++) up[i] = silu(up[i]);

            // Down-project
            matmul_add_bias(ffn_out, up, m->ffn_down_w[l], m->ffn_down_b[l], nf, ne);

            // Residual
            for (int i = 0; i < ne; i++) x[i] += ffn_out[i];
        }
    }

    // Final norm
    layer_norm(x, ne, m->final_norm_w, m->final_norm_b);

    // Output logits
    matmul(out, x, m->lm_head_w, ne, m->n_vocab);
}

// ============================================================================
// Sampler
// ============================================================================

static int sample_argmax(float *logits, int n) {
    int best = 0;
    float best_val = logits[0];
    for (int i = 1; i < n; i++) {
        if (logits[i] > best_val) { best_val = logits[i]; best = i; }
    }
    return best;
}

static int sample_top_k(float *logits, int n, int k, float temperature) {
    // Apply temperature
    if (temperature != 1.0f) {
        for (int i = 0; i < n; i++) logits[i] /= temperature;
    }

    // Simple bubble sort for top-k (k is small)
    typedef struct { int id; float val; } Pair;
    Pair *pairs = llm_alloc(n * sizeof(Pair));
    for (int i = 0; i < n; i++) { pairs[i].id = i; pairs[i].val = logits[i]; }

    for (int i = 0; i < k; i++) {
        for (int j = i + 1; j < n; j++) {
            if (pairs[j].val > pairs[i].val) {
                Pair tmp = pairs[i]; pairs[i] = pairs[j]; pairs[j] = tmp;
            }
        }
    }

    // Softmax over top-k
    float max_val = pairs[0].val;
    float sum = 0.0f;
    for (int i = 0; i < k; i++) {
        pairs[i].val = expf(pairs[i].val - max_val);
        sum += pairs[i].val;
    }

    // Sample from top-k
    float r = 0.5f; // For deterministic behavior without random(), use 0.5
    float cdf = 0.0f;
    for (int i = 0; i < k; i++) {
        cdf += pairs[i].val / sum;
        if (r <= cdf) return pairs[i].id;
    }
    return pairs[k-1].id;
}

// ============================================================================
// Public API
// ============================================================================

int llm_generate(const char *model_path, const char *prompt, char *output, int max_output_len, int max_tokens) {
    llm_reset();

    Model m;
    if (load_model(model_path, &m) < 0) return -1;

    Tokenizer t;
    // Vocab file is model_path with .vocab extension
    char vocab_path[256];
    int i = 0;
    while (model_path[i] && model_path[i] != '.' && i < 250) {
        vocab_path[i] = model_path[i]; i++;
    }
    vocab_path[i] = '\0';
    // Append .vocab
    const char *ext = ".vocab";
    int j = 0;
    while (ext[j]) vocab_path[i++] = ext[j++];
    vocab_path[i] = '\0';

    if (load_tokenizer(vocab_path, &t) < 0) return -1;

    int tokens[512];
    int n_tokens = encode(&t, prompt, tokens, 512);
    if (n_tokens == 0) return -1;

    int pos = n_tokens - 1;
    float *logits = llm_alloc(m.n_vocab * sizeof(float));

    // Initial forward pass through all prompt tokens to populate KV cache
    for (int p = 0; p < n_tokens; p++) {
        transformer(tokens[p], p, &m, logits);
    }

    int gen_tokens[256];
    int n_gen = 0;

    for (int step = 0; step < max_tokens && pos < m.n_ctx - 1; step++) {
        int next_token = sample_top_k(logits, m.n_vocab, 40, 0.8f);
        gen_tokens[n_gen++] = next_token;
        pos++;
        transformer(next_token, pos, &m, logits);
    }

    decode(&t, gen_tokens, n_gen, output, max_output_len);
    return n_gen;
}
