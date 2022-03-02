FROM fedora:31 as builder
RUN dnf install -y \
  coreutils \
  cmake \
  make \
  gcc \
  gcc-c++ \
  ninja-build \
  glib2-devel \
  libevent-devel \
  zlib-devel \
  openssl-devel \
  autoconf \
  automake \
  libzstd-devel \
  xz-devel \
  git \
  gstreamer1-devel \
  gstreamer1-plugins-base-devel
  
WORKDIR /home/donar-build
RUN chown -R 1000 /home/donar-build
USER 1000

RUN git clone --single-branch --branch patch/relay_2 https://gitlab.inria.fr/qdufour/wide-tor.git tor2  && \
  cd ./tor2 && \
  ./autogen.sh && \
  ./configure --disable-asciidoc || cat config.log && \
  make -j`nproc`

RUN git clone --single-branch --branch master https://gitlab.inria.fr/qdufour/wide-tor.git tor3  && \
  cd ./tor3 && \
  ./autogen.sh && \
  ./configure --disable-asciidoc && \
  make -j`nproc`

COPY ./src ./src
COPY CMakeLists.txt .
RUN mkdir out && \
    cd out && \
    cmake -GNinja .. && \
    ninja

#####

FROM fedora:31
RUN dnf install -y \
  glib2 \
  procps-ng \
  valgrind \
  nmap-ncat \
  psmisc \
  libevent \
  zlib \
  strace \
  openssl \
  libzstd \
  xz-libs \
  moreutils \
  gstreamer1 \
  gstreamer1-plugins-base \
  gstreamer1-plugins-good \
  gstreamer1-plugins-bad-free \
  gstreamer1-plugins-ugly-free

WORKDIR /home/donar
RUN mkdir /home/donar/shared && mkdir /home/donar/res && chown -R 1000 /home/donar
USER 1000
ENV HOME /home/donar
COPY --from=builder /home/donar-build/out/donar /usr/local/bin
COPY --from=builder /home/donar-build/out/measlat /usr/local/bin
COPY --from=builder /home/donar-build/out/udpecho /usr/local/bin
COPY --from=builder /home/donar-build/out/torecho /usr/local/bin
COPY --from=builder /home/donar-build/out/dcall /usr/local/bin
COPY --from=builder /home/donar-build/tor2/src/app/tor /usr/local/bin/tor2
COPY --from=builder /home/donar-build/tor3/src/app/tor /usr/local/bin/tor3
COPY ./scripts/container/* /usr/local/bin/
COPY ./torrc_* /etc/
COPY ./assets /assets
