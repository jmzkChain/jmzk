# Run in docker

Simple and fast setup of everiToken on Docker is also available.

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement

- At least 7GB RAM (Docker -> Preferences -> Advanced -> Memory -> 7GB or above)
- If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build evt image

```bash
git clone https://github.com/everitoken/evt.git --recursive  --depth 1
cd evt/Docker
docker build . -t everitoken/evt
```

The above will build off the most recent commit to the master branch by default. If you would like to target a specific branch/tag, you may use a build argument. For example, if you wished to generate a docker image based off of the Aurora-v1.0 tag, you could do the following:

```bash
docker build -t everitoken/evt:Aurora-v1.0 --build-arg branch=Aurora-v1.0 .
```

By default, the root key of evt is set to `EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV`. You can override this using the `rootkey` argument while building the docker image.

```bash
docker built -t everitoken/evt --build-arg rootkey=<rootkey> .
```

## Start evtd docker container only

```bash
docker run --name evtd -p 8888:8888 -p 9876:9876 -t everitoken/evt evtd.sh arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:

```bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' evtd
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container

```bash
docker run --name evtd -v /path-to-data-dir:/opt/evtd/data -p 8888:8888 -p 9876:9876 -t everitoken/evt evtd.sh arg1 arg2
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Start both evtd and evtwd containers

```bash
docker volume create --name=evtd-data-volume
docker volume create --name=evtwd-data-volume
docker-compose up -d
```

After `docker-compose up -d`, two services named `evtd` and `evtwd` will be started. evtd service would expose ports 8888 and 9876 to the host. evtwd service does not expose any port to the host, it is only accessible to evtc when running evtc is running inside the evtwd container as described in "Execute evtc commands" section.

### Execute evtc commands

You can run the `evtc` commands via a bash alias.

```bash
alias evtc='docker-compose exec evtwd /opt/evt/bin/evtc -u http://evtd:8888 --wallet-url http://localhost:9999'
evtc get info
```
If you don't need evtwd afterwards, you can stop the evtwd service using

```bash
docker-compose stop evtwd
```

### Stop and remove all
If you have tested all the features, you can stop and remove all the containers using
```bash
docker-compose down
```

### Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "3"

services:
  evtd:
    volumes:
      - evtd-data-volume:/opt/evt/data
      - ./config2.ini:/opt/evt/etc/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose down
docker-compose up
```

### Clear data directory

The data volume created by docker-compose can be deleted as follows:

```bash
docker volume rm evtd-data-volume
docker volume rm evtwd-data-volume
```

### Docker Hub

Docker Hub image available from [docker hub](https://hub.docker.com/r/everitoken/evt/).
Create a new `docker-compose.yaml` file with the content below

```bash
version: "3"

services:
  builder:
    build:
      context: builder
    image: everitoken/builder:latest

  evtd:
    build:
      context: .
    image: everitoken/evt:latest
    command: /opt/evt/bin/evtd.sh --data-dir /opt/evt/data
    hostname: evtd
    ports:
      - 8888:8888
      - 9876:9876
    expose:
      - "8888"
    volumes:
      - evtd-data-volume:/opt/evt/data

  evtwd:
    image: everitoken/evt:latest
    command: /opt/evt/bin/evtwd --wallet-dir /opt/evt/data
    hostname: evtwd
    links:
      - evtd
    volumes:
      - evtwd-data-volume:/opt/evt/data

volumes:
  evtd-data-volume:
    external: true
  evtwd-data-volume:
    external: true

```

*NOTE:* the default version is the latest, you can change it to what you want

run `docker pull everitoken/evt:latest`

run `docker-compose up`

### About MongoDB Plugin

Currently, the mongodb plugin is disabled in `config.ini` by default, you have to change it manually in `config.ini` or you can mount a `config.ini` file to `/opt/evt/etc/config.ini` in the docker-compose file.
