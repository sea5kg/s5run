# stage 0: build binary
FROM sea5kg/s5run:build-environment-2026-11-24

COPY ./ /root/
WORKDIR /root/
RUN rm -rf tmp && ./build_simple.sh

# RUN ./misc/py_check.sh

RUN ./smoke_tests.py

# stage 1: release
FROM sea5kg/s5run:release-environment-2026-11-24
LABEL "maintainer"="Evgenii Sopov <mrseakg@gmail.com>"
LABEL "repository"="https://github.com/sea5kg/s5run"

# RUN mkdir /root/data
COPY --from=0 /root/s5run /usr/bin/s5run

EXPOSE 7777
CMD ["s5run", "server", "--listen", "0.0.0.0:7777"]
