#This file expects 3 local files: "nordesc2019.xml", "fullchain.pem" and "privkey.pem"

FROM bitnami/minideb:bookworm AS builder

ARG WT_VERSION=4.11.1

RUN install_packages ca-certificates openssl git g++ libssl-dev libxml2-dev libboost-all-dev cmake make

RUN openssl dhparam -out /etc/ssl/dh4096.pem 4096

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
    mkdir -p /opt/Helsebib/MeSHWeb/ && \
    make -j$(nproc) && \
    make install


FROM bitnami/minideb:bookworm AS mesh-import

RUN install_packages libxml2

COPY --from=builder /projects/MeSH/MeSHImport/MeSHImport /projects/MeSH/MeSHImport/nordesc_topnodes.xml /app/
ADD nordesc2019.xml /app/

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

RUN useradd -MU mesh

USER mesh

ADD --chown=mesh:mesh --chmod=600 fullchain.pem /etc/ssl/mesh_fullchain.pem
ADD --chown=mesh:mesh --chmod=600 privkey.pem /etc/ssl/mesh_privkey.pem
COPY --from=builder --chown=mesh:mesh --chmod=600 /etc/ssl/dh4096.pem /etc/ssl/dh4096.pem

COPY --from=builder --chown=mesh:mesh /opt/Helsebib/MeSHWeb /app/
COPY --from=builder --chown=mesh:mesh /usr/local/share/Wt/resources /app/resources/

WORKDIR /app/

ENTRYPOINT ["/app/MeSHWeb", \
            "--docroot", ".", \
            "--config", "/app/wt_config.xml", \
            "--servername", "mesh.uia.no", \
            "--http-listen", "0.0.0.0:8080", \
            "--https-listen", "0.0.0.0:8443", \
            "--ssl-certificate", "/etc/ssl/mesh_fullchain.pem", \
            "--ssl-private-key", "/etc/ssl/mesh_privkey.pem", \
            "--ssl-tmp-dh", "/etc/ssl/dh4096.pem"]
