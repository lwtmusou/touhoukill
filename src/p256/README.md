# `p256` - ECDSA signature verification over P-256

`p256` is a C implementation of ECDSA signature verification over NIST P-256 w/SHA-256
that fits in a single file. It is a minimization of the fantastic BearSSL library
by Thomas Pornin. The file is self-contained (including SHA256, bignum, ECC, ECDSA)
and exposes just a single function with zero dependencies. It is embeddable (runs
on a smallish Cortex-M3 with 20kB of SRAM) and easy to plug into your build system.

The provided function is:

```
p256_ret_t ec_p256_verify(uint8_t *msg, size_t msg_len, uint8_t *sig, const uint8_t *pk);
```

where
 * `msg` is a pointer to the `msg_len`-bytes long message being authenticated
 * `sig` is a 64-byte buffer containing the raw `(r,s)` signature
 * `pk` is a buffer containing the public key in either uncompressed format
    (must be a 65-byte buffer starting with 04) or compressed (must be at least
    33-byte buffer starting with 02 or 03).

The function returns `P256_SUCCESS` iff signature is valid.
Uncompressed keys are made of the constant 04, followed by the 256-bit `x` coordinate,
followed by the 256-bit `y` coordinate. Compressed keys are made of the
constant 02 or 03 (depending on the sign of the y coordinate) followed by
256-bit `x` coordinate.

## Example usage

See [demo.c](demo.c). Include `p256.h`, compile `p256.c` and link it into your application.
You can run the demo as
```
gcc -c p256.c
gcc -o demo demo.c p256.o
./demo
OK
```

## Credits

This repo is basically a repackage of selected pieces of Thomas
Pornin's BearSSL. BearSSL copyright is reproduced below. I also got
inspiration from the series of embedded libraries by the inimitable
[Charles Nicholson](https://github.com/charlesnicholson).

The following people have contributed to `p256` through code, bug
reports, issues or ideas:

* Arthur De Belen

## Developing

### Unit tests

To run unit tests: 

```
make -j && ./build/p256_unittests
[...]
===============================================================================
All tests passed (2 assertions in 1 test case)
```

The unit test is based on [catch2](https://github.com/catchorg/Catch2)
as Charles Nicholson does it. A presubmit workflow compiles
`p256` on macOS and Linux (gcc) 32/64 as a github action.

The following commands might be useful:

### Keygen with OpenSSL

Generate private key:
```
openssl ecparam -name prime256v1 -genkey -noout -out key.pem
```

Export public key:
```
openssl ec -in key.pem -pubout -out public.pem

openssl ec -pubin -in public.pem -text -noout
read EC key
Private-Key: (256 bit)
pub:
    04:56:16:ab:0d:f8:5a:c8:9c:c8:53:b8:4e:53:ca:
    b5:35:22:4a:7d:bc:39:27:02:76:dd:a8:00:85:3e:
    e8:ae:9b:68:b9:53:59:70:4f:87:e0:23:42:4d:5d:
    84:2f:08:21:d8:8c:e0:1f:b6:a8:1a:6a:1c:87:8a:
    81:13:0c:61:68
ASN1 OID: prime256v1
NIST CURVE: P-256
```

### Sign with OpenSSL

Sign a message and dump the raw signature:

```
echo hi > input.bin
openssl dgst -sha256 -sign key.pem input.bin > input.signed

openssl asn1parse -inform der < data.sig
    0:d=0  hl=2 l=  68 cons: SEQUENCE
    2:d=1  hl=2 l=  32 prim: INTEGER           :6C98B6809F6E2C7395C6C9F18A302821C5F60369D3ABD192E9E5C4F607D518D3
   36:d=1  hl=2 l=  32 prim: INTEGER           :4A9D74A0F44C61031330A7E3F27908F5C589FE6427DB7C3F3F7409559E500C3C
```

### Verify with OpenSSL

```
openssl dgst -sha256 -verify public.pem -signature input.signed input.bin
```


### Key gen with a yubikey

To generate signatures with a PKCS11 (such a yubikey), you can do something like

```
yubico-piv-tool -a delete-certificate -s 9c -k
yubico-piv-tool -a generate -s 9c -A ECCP256 -k
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEVharDfhayJzIU7hOU8q1NSJKfbw5
JwJ23agAhT7orptouVNZcE+H4CNCTV2ELwgh2IzgH7aoGmoch4qBEwxhaA==
-----END PUBLIC KEY-----
Successfully generated a new private key.
```

### Sign with a yubikey

```
yubico-piv-tool -a verify-pin --sign -s 9c -H SHA256 -A ECCP256 -i data.txt -o data.sig
openssl dgst -sha256 -verify pubkey.pem -signature data.sig data.txt
```


## Notes

See https://github.com/oreparaz/p256 for the latest version.

__Warning__: This won't win any beauty contests. BearSSL was not designed
to be used like this. For any serious use you should just use BearSSL
without the bastardization that is happening in this repo. If you think
life is too short to fight build systems, and are in a hurry to embed an
ECDSA verification, then I hope you find this useful and wish you good luck 🍀.

## License

```
Copyright (c) 2016 Thomas Pornin <pornin@bolet.org>

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be 
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```