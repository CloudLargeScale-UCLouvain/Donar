# DONAR

## 1) Runtime dependencies

On Fedora:

```bash
sudo dnf install -y \
  glib2 \
  gstreamer1 \
  libevent \
  zlib \
  openssl \
  libzstd \
  xz-libs \
  wget \
  unzip
```

On Ubuntu:

```bash
sudo apt-get install -y \
  libglib2.0-0 \
  libgstreamer1.0 \
  gstreamer1.0-alsa \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-pulseaudio \
  libevent-2.1 \
  zlib1g \
  libssl1.1 \
  zstd \
  liblzma5 \
  wget \
  unzip
```

## 2) Obtain binaries

Download the version you want here: https://cloud.deuxfleurs.fr/d/612993c04e9d40609242/

And extract the zip file:

```bash
unzip donar*.zip
cd release
```

## 3) Callee

In a first terminal:

```bash
./tor2 -f torrc_guard_12
```

In a second terminal:

```bash
./donar -s -a lightning -l 12 -p 'fast_count=3!tick_tock=0!window=2000' -e 5000 -r 5000
```

In a third terminal:

```bash
./dcall 127.13.3.7
```

Your "address" is contained inside the `onion_services.pub`, you must transmit it out of band to people that want to call you.

## 4) Caller

In a first terminal:

```bash
./tor2 -f torrc_guard_12
```

In a second terminal:

```bash
./donar -c -o onion_services.pub -a lightning -l 12 -p 'fast_count=3!tick_tock=0!window=2000' -e 5000 -r 5000
```

In a third terminal:

```bash
./dcall 127.13.3.7
```


