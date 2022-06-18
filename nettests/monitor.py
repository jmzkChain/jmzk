import docker

if __name__ == '__main__':
    client = docker.from_env()
    i = -1
    name = 'jmzkd_'
    while(True):
        try:
            i += 1
            container = client.containers.get('{}{}'.format(name,i))
            print(container.logs(tail=1))
            # container.stop()
            # container.remove()
            # print('free {}{} succeed'.format(name, i))
        except docker.errors.NotFound:
            if(i >= 10):
                break
