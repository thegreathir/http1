from locust import HttpUser, task

class IndexGetterUser(HttpUser):

    @task
    def get_index(self):
        self.client.get('/')