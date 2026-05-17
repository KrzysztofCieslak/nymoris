#!/usr/bin/env python3
"""
Convert HuggingFace/transformers models to Nymoris NYMOLLM format.

Usage:
    python convert.py --model gpt2 --output model.nymollm
    python convert.py --model TinyLlama/TinyLlama-1.1B-Chat-v1.0 --output tinyllama.nymollm

Requires: torch, transformers, numpy
"""

import argparse
import struct
import sys
import os

def write_header(f, config):
    """Write NYMOLLM binary header."""
    f.write(b"NYMOLLM\x00")  # Magic (8 bytes)
    f.write(struct.pack("<I", 1))           # Version
    f.write(struct.pack("<I", config["vocab_size"]))
    f.write(struct.pack("<I", config["n_ctx"]))
    f.write(struct.pack("<I", config["n_embd"]))
    f.write(struct.pack("<I", config["n_head"]))
    f.write(struct.pack("<I", config["n_layer"]))
    f.write(struct.pack("<I", config["n_ff"]))
    f.write(struct.pack("<I", 0))           # dtype: 0 = f32


def write_tensor(f, tensor):
    """Write a tensor as float32."""
    data = tensor.detach().cpu().float().numpy().flatten()
    f.write(data.astype("float32").tobytes())


def estimate_n_ff(n_embd):
    """Estimate FFN hidden size if not in config."""
    # Most models use 4 * n_embd
    return n_embd * 4


def convert_model(model_name, output_path):
    try:
        import torch
        from transformers import AutoModelForCausalLM, AutoTokenizer, AutoConfig
    except ImportError:
        print("Error: requires torch and transformers.")
        print("  pip install torch transformers numpy")
        sys.exit(1)

    print(f"Loading model: {model_name}")
    model = AutoModelForCausalLM.from_pretrained(model_name, torch_dtype=torch.float32)
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    cfg = AutoConfig.from_pretrained(model_name)

    state = model.state_dict()

    # Extract config
    n_vocab = cfg.vocab_size
    n_ctx = getattr(cfg, "max_position_embeddings", getattr(cfg, "n_positions", 2048))
    n_embd = getattr(cfg, "hidden_size", getattr(cfg, "n_embd", 768))
    n_head = getattr(cfg, "num_attention_heads", getattr(cfg, "n_head", 12))
    n_layer = getattr(cfg, "num_hidden_layers", getattr(cfg, "n_layer", 12))
    n_ff = getattr(cfg, "intermediate_size", getattr(cfg, "n_inner", estimate_n_ff(n_embd)))

    config = {
        "vocab_size": n_vocab,
        "n_ctx": n_ctx,
        "n_embd": n_embd,
        "n_head": n_head,
        "n_layer": n_layer,
        "n_ff": n_ff,
    }

    print(f"Config: vocab={n_vocab}, ctx={n_ctx}, embd={n_embd}, "
          f"heads={n_head}, layers={n_layer}, ff={n_ff}")

    # Detect model architecture type
    model_type = cfg.model_type
    print(f"Model type: {model_type}")

    with open(output_path, "wb") as f:
        write_header(f, config)

        # Token embeddings
        tok_emb_key = "transformer.wte.weight" if "transformer.wte.weight" in state else "model.embed_tokens.weight"
        print(f"Writing token embeddings from {tok_emb_key}")
        write_tensor(f, state[tok_emb_key])

        # Position embeddings (if present, else zeros)
        pos_emb_key = "transformer.wpe.weight" if "transformer.wpe.weight" in state else None
        if pos_emb_key and pos_emb_key in state:
            print(f"Writing position embeddings from {pos_emb_key}")
            write_tensor(f, state[pos_emb_key])
        else:
            print("No position embeddings found, writing zeros")
            f.write(b"\x00" * (n_ctx * n_embd * 4))

        # Layer weights
        for layer_idx in range(n_layer):
            print(f"Writing layer {layer_idx + 1}/{n_layer}")

            if model_type in ("gpt2", "gpt_neo", "gptj"):
                # GPT-2 style naming
                prefix = f"transformer.h.{layer_idx}."
                ln1_w = state.get(prefix + "ln_1.weight")
                ln1_b = state.get(prefix + "ln_1.bias")
                attn_q_w = state.get(prefix + "attn.c_attn.weight")
                attn_q_b = state.get(prefix + "attn.c_attn.bias")
                attn_o_w = state.get(prefix + "attn.c_proj.weight")
                attn_o_b = state.get(prefix + "attn.c_proj.bias")
                ln2_w = state.get(prefix + "ln_2.weight")
                ln2_b = state.get(prefix + "ln_2.bias")
                mlp_up_w = state.get(prefix + "mlp.c_fc.weight")
                mlp_up_b = state.get(prefix + "mlp.c_fc.bias")
                mlp_down_w = state.get(prefix + "mlp.c_proj.weight")
                mlp_down_b = state.get(prefix + "mlp.c_proj.bias")

                # GPT-2 uses conv1d weights that are transposed
                if attn_q_w is not None and attn_q_w.dim() == 2:
                    attn_q_w = attn_q_w.t()
                if attn_o_w is not None and attn_o_w.dim() == 2:
                    attn_o_w = attn_o_w.t()
                if mlp_up_w is not None and mlp_up_w.dim() == 2:
                    mlp_up_w = mlp_up_w.t()
                if mlp_down_w is not None and mlp_down_w.dim() == 2:
                    mlp_down_w = mlp_down_w.t()

                # GPT-2 combines Q,K,V into one matrix - split them
                if attn_q_w is not None:
                    qkv = attn_q_w.reshape(n_embd, 3, n_embd)
                    q_w = qkv[:, 0, :]
                    k_w = qkv[:, 1, :]
                    v_w = qkv[:, 2, :]
                else:
                    q_w = k_w = v_w = None

                if attn_q_b is not None:
                    qkv_b = attn_q_b.reshape(3, n_embd)
                    q_b = qkv_b[0]
                    k_b = qkv_b[1]
                    v_b = qkv_b[2]
                else:
                    q_b = k_b = v_b = None

            elif model_type in ("llama", "mistral", "tinyllama"):
                # Llama style naming
                prefix = f"model.layers.{layer_idx}."
                ln1_w = state.get(prefix + "input_layernorm.weight")
                ln1_b = None
                q_w = state.get(prefix + "self_attn.q_proj.weight")
                q_b = None
                k_w = state.get(prefix + "self_attn.k_proj.weight")
                k_b = None
                v_w = state.get(prefix + "self_attn.v_proj.weight")
                v_b = None
                attn_o_w = state.get(prefix + "self_attn.o_proj.weight")
                attn_o_b = None
                ln2_w = state.get(prefix + "post_attention_layernorm.weight")
                ln2_b = None
                mlp_up_w = state.get(prefix + "mlp.up_proj.weight")
                mlp_up_b = None
                mlp_down_w = state.get(prefix + "mlp.down_proj.weight")
                mlp_down_b = None

                # Transpose if needed
                if q_w is not None and q_w.dim() == 2:
                    q_w = q_w.t()
                if k_w is not None and k_w.dim() == 2:
                    k_w = k_w.t()
                if v_w is not None and v_w.dim() == 2:
                    v_w = v_w.t()
                if attn_o_w is not None and attn_o_w.dim() == 2:
                    attn_o_w = attn_o_w.t()
                if mlp_up_w is not None and mlp_up_w.dim() == 2:
                    mlp_up_w = mlp_up_w.t()
                if mlp_down_w is not None and mlp_down_w.dim() == 2:
                    mlp_down_w = mlp_down_w.t()

            else:
                print(f"Unsupported model type: {model_type}")
                sys.exit(1)

            # Write attention norm
            write_tensor(f, ln1_w if ln1_w is not None else torch.zeros(n_embd))
            write_tensor(f, ln1_b if ln1_b is not None else torch.zeros(n_embd))

            # Write Q, K, V weights and biases
            write_tensor(f, q_w if q_w is not None else torch.zeros(n_embd, n_embd))
            write_tensor(f, q_b if q_b is not None else torch.zeros(n_embd))
            write_tensor(f, k_w if k_w is not None else torch.zeros(n_embd, n_embd))
            write_tensor(f, k_b if k_b is not None else torch.zeros(n_embd))
            write_tensor(f, v_w if v_w is not None else torch.zeros(n_embd, n_embd))
            write_tensor(f, v_b if v_b is not None else torch.zeros(n_embd))

            # Write output projection
            write_tensor(f, attn_o_w if attn_o_w is not None else torch.zeros(n_embd, n_embd))
            write_tensor(f, attn_o_b if attn_o_b is not None else torch.zeros(n_embd))

            # Write FFN norm
            write_tensor(f, ln2_w if ln2_w is not None else torch.zeros(n_embd))
            write_tensor(f, ln2_b if ln2_b is not None else torch.zeros(n_embd))

            # Write FFN up and down
            write_tensor(f, mlp_up_w if mlp_up_w is not None else torch.zeros(n_embd, n_ff))
            write_tensor(f, mlp_up_b if mlp_up_b is not None else torch.zeros(n_ff))
            write_tensor(f, mlp_down_w if mlp_down_w is not None else torch.zeros(n_ff, n_embd))
            write_tensor(f, mlp_down_b if mlp_down_b is not None else torch.zeros(n_embd))

        # Final norm
        if model_type in ("gpt2", "gpt_neo", "gptj"):
            final_ln_w = state.get("transformer.ln_f.weight")
            final_ln_b = state.get("transformer.ln_f.bias")
        else:
            final_ln_w = state.get("model.norm.weight")
            final_ln_b = None

        write_tensor(f, final_ln_w if final_ln_w is not None else torch.zeros(n_embd))
        write_tensor(f, final_ln_b if final_ln_b is not None else torch.zeros(n_embd))

        # LM head (often shared with tok_emb)
        if model_type in ("gpt2", "gpt_neo", "gptj"):
            lm_head = state.get("transformer.wte.weight")  # Shared in GPT-2
        else:
            lm_head = state.get("lm_head.weight", state.get("model.embed_tokens.weight"))

        if lm_head is not None and lm_head.dim() == 2:
            lm_head = lm_head.t()
        write_tensor(f, lm_head if lm_head is not None else torch.zeros(n_embd, n_vocab))

    print(f"Model written to: {output_path}")

    # Write vocab file
    vocab_path = output_path + ".vocab"
    write_vocab(tokenizer, vocab_path, n_vocab)
    print(f"Vocab written to: {vocab_path}")

    # Report size
    size_mb = os.path.getsize(output_path) / (1024 * 1024)
    print(f"Model size: {size_mb:.1f} MB")


def write_vocab(tokenizer, vocab_path, n_vocab):
    """Write a simple byte-level vocabulary file."""
    # For now, write a simple byte-level vocab that works with any text
    # Each token is a byte sequence.
    # We extract subword tokens from the tokenizer if possible.

    vocab = []
    if hasattr(tokenizer, "get_vocab"):
        vocab_dict = tokenizer.get_vocab()
        # Sort by token id
        id_to_token = sorted(vocab_dict.items(), key=lambda x: x[1])
        for token_str, _ in id_to_token:
            if isinstance(token_str, str):
                # Encode to bytes using utf-8
                vocab.append(token_str.encode("utf-8", errors="replace"))
            else:
                vocab.append(bytes([0]))
    else:
        # Fallback: byte-level vocab
        vocab = [bytes([i]) for i in range(256)]

    # Trim or pad to n_vocab
    while len(vocab) < n_vocab:
        vocab.append(bytes([0]))
    vocab = vocab[:n_vocab]

    with open(vocab_path, "wb") as f:
        f.write(struct.pack("<I", len(vocab)))
        for token_bytes in vocab:
            length = min(len(token_bytes), 255)
            f.write(struct.pack("B", length))
            f.write(token_bytes[:length])


def main():
    parser = argparse.ArgumentParser(description="Convert HF model to NYMOLLM format")
    parser.add_argument("--model", required=True, help="HuggingFace model name or path")
    parser.add_argument("--output", required=True, help="Output file path")
    args = parser.parse_args()

    convert_model(args.model, args.output)


if __name__ == "__main__":
    main()
