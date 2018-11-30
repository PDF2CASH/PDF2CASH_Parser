FROM buildpack-deps:stretch

RUN mkdir /parser

ADD . /parser/

WORKDIR /parser/build/

EXPOSE 10022

CMD ["./Server"]


