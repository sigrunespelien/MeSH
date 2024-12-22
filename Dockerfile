FROM debian:bookworm-slim AS builder

ARG WT_VERSION=4.11.1
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get --assume-yes update && \
    apt-get --assume-yes dist-upgrade && \
    apt-get --assume-yes install git g++ libssl-dev libxml2-dev libboost-all-dev cmake make

WORKDIR /projects

ADD https://github.com/emweb/wt/archive/${WT_VERSION}.tar.gz .

RUN gunzip ${WT_VERSION}.tar.gz && \
    tar xf ${WT_VERSION}.tar && \
    rm ${WT_VERSION}.tar && \
    ln -s wt-${WT_VERSION} wt

WORKDIR /projects/wt/build

RUN cmake ../ -DENABLE_LIBWTDBO:BOOL=OFF && \
    make -j2 && \
    make install && \
    ldconfig

WORKDIR /projects

RUN git clone -b feature/docker --single-branch https://github.com/sigrunespelien/MeSH.git

WORKDIR /projects/MeSH/MeSHImport

RUN git clone https://github.com/frodegill/cpp-elasticsearch.git && \
    make -j2

WORKDIR /projects/MeSH/MeSHWeb

RUN ln -sf ../MeSHImport/cpp-elasticsearch . && \
    ln -sf /usr/local/share/Wt/resources . && \
    ln -sf /usr/local/share/Wt/resources /opt/Helsebib/MeSHWeb/ && \
    make -j2 && \
    make install


FROM bitnami/minideb:bookworm AS mesh-import

COPY --from=builder /usr/local/bin/MeSHImport /projects/MeSH/MeSHImport/nordesc_topnodes.xml /app
COPY nordesc2019.xml /app

ENTRYPOINT ["/app/MeSHImport" "elasticsearch:9200" "--clean" "--topnodes" "/app/nordesc_topnodes.xml" "/app/nordesc2019.xml"]

FROM bitnami/minideb:bookworm AS mesh

RUN install_packages unattended-upgrades

COPY --from=builder /usr/local/lib/libwt* /usr/local/lib/
