""" A directory fileserver

This module uses inotify to watch events on the filesystem. Files are synced
with rsync from PDC and when they arrive at the filesystem they are imported to
the database.

"""
from os.path import isdir
import Queue

import logging
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

from pyinotify import (   # pylint: disable=no-name-in-module
    WatchManager, ProcessEvent, ThreadedNotifier, IN_CREATE, IN_MOVED_TO
)
from odincal.level0_file_importer import Level0File
from odincal.config import config

logging.basicConfig(level=logging.DEBUG)


class EventHandler(ProcessEvent):
    """Event handler"""

    def __init__(self, queue):
        self.logger = logging.getLogger('level1b.filesystem_list')
        self.logger.debug('started filesystem sensor')
        self.queue = queue
        ProcessEvent.__init__(self)

    def process_IN_CREATE(self, event):  # pylint: disable=invalid-name
        """A file is created in the watched directory"""
        if isdir(event.pathname):
            self.logger.info(
                "Detected new directory: %s",
                event.pathname
            )
        else:
            self.logger.info(
                'Adding (CREATE) to processing queue: %s',
                event.pathname
            )
            self.queue.put(event.pathname)

    def process_IN_MOVED_TO(self, event):  # pylint: disable=invalid-name
        """A file is moved to to the watched directory"""
        self.logger.info(
            'Adding (MOVED) to processing queue: %s',
            event.pathname
        )
        self.queue.put(event.pathname)


class Importer(object):
    """ Responsible for pulling the queue and run the import command
    """

    def __init__(self, queue, session):
        self.queue = queue
        self.session = session
        self.logger = logging.getLogger(__name__)
        self.level0 = None

    def import_to_database(self, newfile):
        """ Run the specific command to import the file"""
        self.logger.info('processing file %s', newfile)
        try:
            self.level0 = Level0File(newfile)
        except ValueError:
            self.logger.info('skipping file %s', newfile)
        else:
            self.logger.info('adding to db %s', newfile)
            self.session.merge(self.level0)
            self.session.commit()

    def import_files(self):
        """ Import all files in the queue
        """
        while True:
            try:
                newfile = self.queue.get()
                self.import_to_database(newfile)
                self.queue.task_done()
            except Queue.Empty:
                self.logger.warn('Queue is empty')
                break
            except KeyboardInterrupt:
                self.logger.warn('C-c pressed exiting')
                break


def main():
    """set up and run the fileserver"""
    logger = logging.getLogger(__name__)
    logger.info('starting testserver')
    queue = Queue.Queue()
    manager = WatchManager()
    handler = EventHandler(queue)
    notifier = ThreadedNotifier(manager, handler)
    directory = '/odindata/odin/level0'
    manager.add_watch(
        directory,
        IN_CREATE | IN_MOVED_TO,
        rec=True,
        auto_add=True,
        quiet=False
    )
    logger.info('watching %s', directory)

    # setup database session
    engine = create_engine(config.get('database', 'connect_string'))
    session_factory = sessionmaker(bind=engine)
    session = session_factory()
    # Run the service
    notifier.start()
    importer = Importer(queue, session)
    importer.import_files()
    session.close()
    notifier.stop()


if __name__ == '__main__':
    main()
