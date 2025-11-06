import os
import subprocess

from .celery_base import CeleryBaseService

__all__ = ['CeleryDefaultService']


class CeleryDefaultService(CeleryBaseService):

    def __init__(self, **kwargs):
        kwargs['queue'] = 'celery'
        super().__init__(**kwargs)

    def open_subprocess(self):
        env = os.environ.copy()
        env['SERVER_NAME'] = 'celery'
        kwargs = {
            'cwd': self.cwd,
            'stderr': self.log_file,
            'stdout': self.log_file,
            'env': env
        }
        self._process = subprocess.Popen(self.cmd, **kwargs)
