#!/usr/bin/env python3
"""Generate a minimal NYMOLLM model for testing in QEMU."""

import struct
import random
import argparse
import math

def generate_model(output_path, vocab_size=256, n_ctx=64, n_embd=32, n_head=2, n_layer=1):
    """Generate a tiny random model in NYMOLLM format."""
    n_ff = n_embd * 4
    hs = n_embd // n_head

    print(f"Generating test model: vocab={vocab_size}, ctx={n_ctx}, embd={n_embd}, heads={n_head}, layers={n_layer}")

    with open(output_path, "wb") as f:
        # Header
        f.write(b"NYMOLLM\x00")
        f.write(struct.pack("<I", 1))           # version
        f.write(struct.pack("<I", vocab_size))
        f.write(struct.pack("<I", n_ctx))
        f.write(struct.pack("<I", n_embd))
        f.write(struct.pack("<I", n_head))
        f.write(struct.pack("<I", n_layer))
        f.write(struct.pack("<I", n_ff))
        f.write(struct.pack("<I", 0))           # dtype = f32

        def write_rand(shape):
            total = 1
            for s in shape:
                total *= s
            for _ in range(total):
                val = random.gauss(0, 1) * 0.02
                f.write(struct.pack("<f", val))

        def write_zeros(shape):
            total = 1
            for s in shape:
                total *= s
            for _ in range(total):
                f.write(struct.pack("<f", 0.0))

        # Token embeddings
        write_rand((vocab_size, n_embd))

        # Position embeddings (zeros)
        write_zeros((n_ctx, n_embd))

        for l in range(n_layer):
            # attn norm
            write_rand((n_embd,))
            write_zeros((n_embd,))

            # Q, K, V weights + biases
            write_rand((n_embd, n_embd))
            write_zeros((n_embd,))
            write_rand((n_embd, n_embd))
            write_zeros((n_embd,))
            write_rand((n_embd, n_embd))
            write_zeros((n_embd,))

            # Output projection
            write_rand((n_embd, n_embd))
            write_zeros((n_embd,))

            # FFN norm
            write_rand((n_embd,))
            write_zeros((n_embd,))

            # FFN up + down
            write_rand((n_embd, n_ff))
            write_zeros((n_ff,))
            write_rand((n_ff, n_embd))
            write_zeros((n_embd,))

        # Final norm
        write_rand((n_embd,))
        write_zeros((n_embd,))

        # LM head (shared with tok_emb, but write separate for simplicity)
        write_rand((n_embd, vocab_size))

    import os
    size_kb = os.path.getsize(output_path) / 1024
    print(f"Model written to {output_path} ({size_kb:.1f} KB)")

    # Write vocab file
    vocab_path = output_path + ".vocab"
    with open(vocab_path, "wb") as f:
        f.write(struct.pack("<I", vocab_size))
        for i in range(vocab_size):
            # Simple byte-level vocab: token i = byte i
            f.write(struct.pack("B", 1))
            f.write(struct.pack("B", i))
    print(f"Vocab written to {vocab_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="test_model.nymollm")
    parser.add_argument("--vocab", type=int, default=256)
    parser.add_argument("--ctx", type=int, default=64)
    parser.add_argument("--embd", type=int, default=32)
    parser.add_argument("--head", type=int, default=2)
    parser.add_argument("--layer", type=int, default=1)
    args = parser.parse_args()

    generate_model(args.output, args.vocab, args.ctx, args.embd, args.head, args.layer)
