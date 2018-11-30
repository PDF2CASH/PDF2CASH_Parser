[![Build Status](https://travis-ci.org/PDF2CASH/PDF2CASH_Parser.svg?branch=development)](https://travis-ci.org/PDF2CASH/PDF2CASH_Parser)
[![Coverage Status](https://coveralls.io/repos/github/PDF2CASH/PDF2CASH_Parser/badge.svg?branch=development)](https://coveralls.io/github/PDF2CASH/PDF2CASH_Parser?branch=master)


# PDF2CASH_Parser

## Como compilar
Antes de compilar, é necessário a instalão dos pré-requisitos.

### Pré-requisitos

- Qt 5.11.2

### 1. Poppler
Vá na pasta do Poppler, é encontrada no diretório:

`PROJDIR/3rdparty/poppler-library`

Logo após, execute os comandos abaixo para compilação.:

`$ qmake poppler-library.pro`
`$ make`

E instale.:

`$ sudo make install`

> PROJDIR = pasta root do projeto onde foi clonado.


### 2. Servidor
Depois de ter compilado todas bibliotecas necessárias no projeto, agora por último, será o processo da compilação do servidor do parser.

Vá na pasta do Servidor, é encontrado no diretório:

`PROJDIR/server`

E compile-o.:

`qmake -server.pro`
`make`

> PROJDIR = pasta root do projeto onde foi clonado.

## Contribuindo

Por favor leia o nosso [CONTRIBUTING.md](https://github.com/fga-eps-mds/2018.2-PDF2CASH/blob/master/CONTRIBUTING.md) se deseja contribuir com nosso projeto.

## Licensa

Esse projeto é licensiado sobre a licensa MIT
