import os

from locust import TaskSet, task
from locust.contrib.fasthttp import FastHttpLocust
from trafficgen.utils import Reader


regions = []
rindex = 0


def get_traffic(folder):
    assert os.path.exists(folder)
    if len(regions) == 0:
        files = os.listdir(folder)
        if len(files) == 0:
            raise Exception("There's not any traffic files")

        for file in files:
            if file.endswith('_traffic_data.lz4'):
                region = os.path.basename(file)[:2]
                regions.append((region, os.path.join(folder, file)))

    global rindex
    assert rindex < len(regions)
    region, file = regions[rindex]
    rindex += 1

    return region, Reader(file)


class EVTTaskSet(TaskSet):
    def __init__(self, parent):
        super().__init__(parent)
        self.reader = None
        self.region = None
        self.stop = False

    def on_start(self):
        folder = self.locust.user_config.folder
        self.region, self.reader = get_traffic(folder)

    @task(1)
    def push_trx(self):
        if self.stop:
            return

        try:
            trx = self.reader.read_trx()
        except:
            print('{} Traffic has been sent completed'.format(self.region))
            self.stop = True
            return

        with self.client.post('/v1/chain/push_transaction', trx, catch_response=True) as response:
            if response.status_code == 202:
                response.success()


class WebsiteUser(FastHttpLocust):
    task_set = EVTTaskSet
    min_wait = 0
    max_wait = 0
