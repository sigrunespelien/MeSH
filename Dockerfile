FROM bitnami/minideb:bookworm AS builder

ARG WT_VERSION=4.11.1

RUN install_packages ca-certificates git g++ libssl-dev libxml2-dev libboost-all-dev cmake make

WORKDIR /projects

ADD https://github.com/emweb/wt/archive/${WT_VERSION}.tar.gz .

RUN gunzip ${WT_VERSION}.tar.gz && \
    tar xf ${WT_VERSION}.tar && \
    rm ${WT_VERSION}.tar && \
    ln -s wt-${WT_VERSION} wt

WORKDIR /projects/wt/build

RUN cmake ../ -DENABLE_LIBWTDBO:BOOL=OFF && \
    make -j$(nproc) && \
    make install

WORKDIR /projects

RUN git clone -b feature/docker --single-branch https://github.com/sigrunespelien/MeSH.git

WORKDIR /projects/MeSH/MeSHImport

RUN git clone https://github.com/frodegill/cpp-elasticsearch.git && \
    make -j$(nproc)

WORKDIR /projects/MeSH/MeSHWeb

RUN ln -sf ../MeSHImport/cpp-elasticsearch . && \
    ln -sf /usr/local/share/Wt/resources . && \
    mkdir -p /opt/Helsebib/MeSHWeb/ && \
    ln -sf /usr/local/share/Wt/resources /opt/Helsebib/MeSHWeb/ && \
    make -j$(nproc) && \
    make install


FROM bitnami/minideb:bookworm AS mesh-import

RUN install_packages libxml2

COPY --from=builder /projects/MeSH/MeSHImport/MeSHImport /projects/MeSH/MeSHImport/nordesc_topnodes.xml /app/
COPY nordesc2019.xml /app/

ENTRYPOINT ["/app/MeSHImport", "elasticsearch:9200", "--clean", "--topnodes", "/app/nordesc_topnodes.xml", "/app/nordesc2019.xml"]


FROM bitnami/minideb:bookworm AS mesh

ARG BOOST_VERSION=1.74.0

RUN install_packages unattended-upgrades libssl3 libxml2 \
                     libboost-locale${BOOST_VERSION} \
                     libboost-system${BOOST_VERSION} \
                     libboost-filesystem${BOOST_VERSION} \
                     libboost-program-options${BOOST_VERSION}

RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/local-lib.conf
COPY --from=builder /usr/local/lib/libwt* /usr/local/lib/
RUN ldconfig

COPY --from=builder /usr/local/share/Wt/resources /usr/local/share/Wt/

RUN useradd -MU mesh

USER mesh

COPY --from=builder /opt/Helsebib/MeSHWeb /app

ENTRYPOINT ["/app/MeSHWeb", "--docroot", ".", "--config", "/app/wt_config.xml", "--http-listen", "0.0.0.0:8443"]
