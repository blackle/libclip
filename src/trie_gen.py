#!/usr/bin/env python3

import os
from functools import lru_cache

import regex as re
import struct
import sys

@lru_cache()
def default_bpe():
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), "bpe_simple_vocab_small.txt")

@lru_cache()
def bytes_to_unicode():
    """
    Returns list of utf-8 byte and a corresponding list of unicode strings.
    The reversible bpe codes work on unicode strings.
    This means you need a large # of unicode characters in your vocab if you want to avoid UNKs.
    When you're at something like a 10B token dataset you end up needing around 5K for decent coverage.
    This is a signficant percentage of your normal, say, 32K bpe vocab.
    To avoid that, we want lookup tables between utf-8 bytes and unicode strings.
    And avoids mapping to whitespace/control characters the bpe code barfs on.
    """
    bs = list(range(ord("!"), ord("~")+1))+list(range(ord("¡"), ord("¬")+1))+list(range(ord("®"), ord("ÿ")+1))
    cs = bs[:]
    n = 0
    for b in range(2**8):
        if b not in bs:
            bs.append(b)
            cs.append(2**8+n)
            n += 1
    cs = [chr(n) for n in cs]
    return dict(zip(bs, cs))


byte_encoder = bytes_to_unicode()
byte_decoder = {v: k for k, v in byte_encoder.items()}
merges = open(default_bpe()).read().split('\n')
merges = merges[1:49152-256-2+1]
merges = [tuple(merge.split()) for merge in merges]
vocab = list(bytes_to_unicode().values())
vocab = vocab + [v+'</w>' for v in vocab]
for merge in merges:
    vocab.append(''.join(merge))
# vocab.extend(['<|startoftext|>', '<|endoftext|>'])
vocab = [bytes([byte_decoder[token] for token in v.replace("</w>", "Ā")]) for v in vocab]
encoder = dict(zip(vocab, range(len(vocab))))
print("#include <stdint.h>")
print("#include <stddef.h>")
print(f"#define START_OF_TEXT {len(vocab)}")
print(f"#define END_OF_TEXT {len(vocab)+1}")

merges = [b'\0'.join([bytes([byte_decoder[token] for token in tokens.replace("</w>", "Ā")]) for tokens in merge]) for merge in merges]
bpe_ranks = dict(zip(merges, range(len(merges))))
# cache = {'<|startoftext|>': '<|startoftext|>', '<|endoftext|>': '<|endoftext|>'}
# pat = re.compile(r"""<\|startoftext\|>|<\|endoftext\|>|'s|'t|'re|'ve|'m|'ll|'d|[\p{L}]+|[\p{N}]|[^\s\p{L}\p{N}]+""", re.IGNORECASE)
# encoderTrie = marisa_trie.BinaryTrie(vocab)
# encoderTrie.save('encoder.marisa')

# mergesTrie = marisa_trie.BinaryTrie(merges)
# mergesTrie.save('merges.marisa')

@lru_cache()
def fibonacci_up_to(v):
    i = 2
    j = 1
    idx = 0
    while i <= v:
        i += j
        j = i-j
        idx += 1
    return (idx, j)

@lru_cache()
def fibonacci_encoding(v):
    v += 1
    bits = ['0' for i in range(fibonacci_up_to(v)[0]+1)]
    while v > 0:
        idx, fib = fibonacci_up_to(v)
        bits[idx] = '1'
        v -= fib
    return ''.join(bits) + '1'

def bits_to_bytes(bits):
    padding = 0 -  (-len(bits)//8 * 8) - len(bits)
    bits += '0' * padding
    return bytes([int("".join(map(str, bits[i:i+8])), 2) for i in range(0, len(bits), 8)])

# def align(b, alignment):
#     padding = 0 - (-len(b)//(2**alignment) * (2**alignment)) - len(b)
#     return b + (b'\x00' * padding)


def make_trie(keys):
    chars = {}
    for key in keys:
        for tok in key:
            if tok not in chars:
                chars[tok] = 0
            chars[tok] += 1
    chars = [x[0] for x in sorted(chars.items(), key=lambda x: -x[1])]
    charlist = chars
    chars = dict(zip(chars, range(len(chars))))

    keys = list(enumerate(keys))


    def dictify(keys):
        return {p: [(i,k[1:]) if len(k) > 1 else i for i,k in keys if k[0] == p] for p in {key[0] for idx,key in keys}}

    def trieify(keys, level):
        # if len(keys) == 1 and isinstance(keys[0], tuple):
        #     v, key = keys[0]
        #     return bits_to_bytes('1111' + fibonacci_encoding(v) + fibonacci_encoding(len(key))+ ''.join(fibonacci_encoding(chars[tok]) for tok in key)) 

        val = None

        prefix = b''
        d = None
        while (d is None or len(d) == 1) and (val is None or len(val) == 0):
            if d is not None:
                key = list(d.keys())[0]
                prefix += bytes([key])
                keys = d[key]
            val = [i for i in keys if not isinstance(i, tuple)]
            keys = [k for k in keys if isinstance(k, tuple)]
            d = dictify(keys)
        assert(val is None or len(val) == 0 or len(val) == 1)

        offsets = []

        out = b''
        for k,v in sorted(d.items(), key=lambda v: chars[v[0]]):
            subtrie = trieify(v, level+1)
            # subtrie = align(subtrie, alignment)
            offsets.append((k, len(subtrie)))
            out += subtrie

        head = ''
        # if len(val) > 0 and len(prefix) == 0 and len(offsets) == 0:
        #     return bits_to_bytes('0' + fibonacci_encoding(val[0]))
        if len(val) > 0 and len(prefix) > 0:
            head += '0'
        elif len(val) > 0:
            head += '10'
        elif len(prefix) > 0:
            head += '110'
        else:
            head += '111'
        total = 0

        if len(prefix) > 0:
            head += fibonacci_encoding(len(prefix))+ ''.join(fibonacci_encoding(chars[tok]) for tok in prefix)
        if len(val) > 0:
            head += fibonacci_encoding(val[0])
            # print(f"value: {val[0]}", file=sys.stderr)
        head += fibonacci_encoding(len(offsets))
        lastchar = None
        lasttotal = 0
        idx = 0
        for k,offset in offsets:
            # if level == 0:
            #     print("child: %d\n\tch: %d\n\toffset: %d\n" % (idx, k, total), file=sys.stderr)
            if lastchar is not None:
                head += fibonacci_encoding(chars[k] - lastchar)
            else:
                head += fibonacci_encoding(chars[k])
            lastchar = chars[k]
            if total != 0: #first node is clearly right after the header data
                head += fibonacci_encoding(total - lasttotal)
            lasttotal = total
            total += offset
            idx += 1

        return bits_to_bytes(head) + out

    return trieify(keys, 0), charlist

def make_header(keys, name):
    t, chars = make_trie(keys)

    data = '{'+','.join(str(int(tok)) for tok in t)+'}'
    chars = '{'+','.join(str(c) for c in chars)+'}'

    print(f"const size_t {name}_length = {len(t)};")
    print(f"const uint8_t {name}_data[] = {data};")
    print(f"const uint8_t {name}_chars[] = {chars};")

make_header(vocab, "vocab")
make_header(merges, "merges")