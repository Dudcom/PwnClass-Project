# PwnClass Project: ChainBoard

ChainBoard is a small HTTP message board backed by a hash chain, with a Linux
and a Windows implementation. It is intentionally vulnerable and built for a
classroom exploitation lab. Run it only on an isolated lab network.

## Layout

```
linux/server.c     Linux implementation
linux/Makefile     builds chainboard (8080) and chainboard-pie (8081)
linux/Dockerfile   32-bit build/run image
windows/server.c   Windows implementation (Winsock)
windows/Makefile   builds chainboard.exe (8080) with mingw-w64
```

## Builds

| Binary           | Arch   | Protections                                   |
|------------------|--------|-----------------------------------------------|
| `chainboard`     | x86-32 | no PIE, executable stack, no canary           |
| `chainboard-pie` | x86-32 | PIE, NX, no canary (run with ASLR on)         |
| `chainboard.exe` | x86-32 | no ASLR, no DEP, no SafeSEH, cookie on (except `handle_message`), image base `0x11220000` |

## Build and run

Linux, on a host with a 32-bit toolchain (`gcc-multilib`):

```
cd linux && make
./chainboard          # port 8080
./chainboard-pie      # port 8081
```

Linux, anywhere Docker runs:

```
docker build --platform linux/386 -t chainboard-linux linux/
docker run --rm --platform linux/386 -p 8080:8080 chainboard-linux
docker run --rm --platform linux/386 -p 8081:8081 chainboard-linux ./chainboard-pie
```

Windows, cross-compiled with mingw-w64 (`apt install gcc-mingw-w64-i686` or
`brew install mingw-w64`):

```
cd windows && make
make gadgets          # print helper addresses
```

Copy `chainboard.exe` to a Windows VM and run it. It needs no DLLs.

## Quick check

```
curl 'http://HOST:8080/?msg=hello+world'
```

The page should list the genesis block and your message, each with its hash.
