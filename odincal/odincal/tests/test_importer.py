"""tester"""
from unittest import TestCase
from Queue import Queue
from mockito import when, mock, verifyNoMoreInteractions, verify
from odincal.level0_fileserver import Importer


class ImportTestCase(TestCase):
    """Test for imports"""
    @classmethod
    def test_loop(cls):
        """imports from queue"""
        queue = Queue()
        queue.put('first')
        queue.put('last')
        session = mock()
        when(Importer).import_to_database('first')
        when(Importer).import_to_database('last').thenRaise(KeyboardInterrupt)
        importer = Importer(queue, session)
        importer.import_files()
        verifyNoMoreInteractions(session)

    @classmethod
    def test_import(cls):
        """imports from queue"""
        filename = '/long/path/to/file/1a5ed04b.fba'
        session = mock()
        queue = mock()
        importer = Importer(queue, session)
        importer.import_to_database(filename)
        verify(session).merge(importer.level0)
        verify(session).commit()
        verifyNoMoreInteractions(session)

    @classmethod
    def test_error_import(cls):
        """imports from queue"""
        filename = '/long/path/to/file/1a5ed04b.txt'
        session = mock()
        queue = mock()
        importer = Importer(queue, session)
        importer.import_to_database(filename)
        verifyNoMoreInteractions(session)
