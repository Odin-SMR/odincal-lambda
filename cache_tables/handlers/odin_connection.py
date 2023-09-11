from collections import namedtuple

from psycopg2 import connect


def odin_connection(credentials: namedtuple):
    """Connects to the database, returns a connection"""
    connection_string = (
        "host={0} ".format(credentials.host) +
        "dbname={0} ".format(credentials.db) +
        "user={0} ".format(credentials.user) +
        "password={0} ".format(credentials.password) +
        "sslmode=verify-ca"
    )
    connection = connect(connection_string)
    return connection
