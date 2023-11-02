import re

pattern = re.compile(r"\s+")


def squeeze_query(query):
    log_friendly_query = re.sub(pattern, " ", query)
    return log_friendly_query.strip()
