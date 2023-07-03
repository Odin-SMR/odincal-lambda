import cPickle
import logging
import logging.handlers
import SocketServer
import struct
import ConfigParser
import os.path
import pkg_resources


class LogRecordStreamHandler(SocketServer.StreamRequestHandler):
    """Handler for a streaming logging request.

    This basically logs the record using whatever logging policy is
    configured locally.
    """

    def handle(self):
        """
        Handle multiple requests - each expected to be a 4-byte length,
        followed by the LogRecord in pickle format. Logs the record
        according to whatever policy is configured locally.
        """
        while True:
            chunk = self.connection.recv(4)
            if len(chunk) < 4:
                break
            slen = struct.unpack(">L", chunk)[0]
            chunk = self.connection.recv(slen)
            while len(chunk) < slen:
                chunk = chunk + self.connection.recv(slen - len(chunk))
            obj = self.unPickle(chunk)
            record = logging.makeLogRecord(obj)
            self.handleLogRecord(record)

    def unPickle(self, data):
        return cPickle.loads(data)

    def handleLogRecord(self, record):
        # if a name is specified, we use the named logger rather than the one
        # implied by the record.
        if self.server.logname is not None:
            name = self.server.logname
        else:
            name = record.name
        logger = logging.getLogger(name)
        # N.B. EVERY record gets logged. This is because Logger.handle
        # is normally called AFTER logger-level filtering. If you want
        # to do filtering, do it at the client end to save wasting
        # cycles and network bandwidth!
        logger.handle(record)


class LogRecordSocketReceiver(SocketServer.ThreadingTCPServer):
    """simple TCP socket-based logging receiver suitable for testing.
    """

    allow_reuse_address = 1

    def __init__(self, host='0.0.0.0',
                 port=logging.handlers.DEFAULT_TCP_LOGGING_PORT,
                 handler=LogRecordStreamHandler):
        SocketServer.ThreadingTCPServer.__init__(self, (host, port), handler)
        self.abort = 0
        self.timeout = 1
        self.logname = None

    def serve_until_stopped(self):
        import select
        abort = 0
        while not abort:
            rd, wr, ex = select.select([self.socket.fileno()],
                                       [], [],
                                       self.timeout)
            if rd:
                self.handle_request()
            abort = self.abort


def main():
    parser = ConfigParser.SafeConfigParser({
        # default values if no configfiles exists
        'logserver_port': '9020',
        'logserver_file': os.path.expanduser('~/.odin/systemlog'),
        'logserver_backupcount': '20',
    })
    parser.add_section('logserver')
    config_files = parser.read([
        pkg_resources.resource_filename("odincal", "logserver_defaults.cfg"),
        os.path.expanduser('~/.odin/odincal.cfg'),
        os.path.expanduser('~/.odin/logserver.cfg'),
    ])
    logger = logging.getLogger()
    fh = logging.handlers.RotatingFileHandler(
        parser.get('logserver', 'logserver_file'),
        mode='a',
        maxBytes=2**20,
        backupCount=parser.getint('logserver', 'logserver_backupcount'),
        encoding=None,
        delay=0
    )
    formatter = logging.Formatter(
        fmt="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        datefmt='%y-%m-%dZ%H:%M:%S')
    fh.setFormatter(formatter)
    logger.addHandler(fh)
    tcpserver = LogRecordSocketReceiver(
        port=parser.getint(
            'logserver', 'logserver_port'))
    tcpserver.serve_until_stopped()


if __name__ == "__main__":
    main()
