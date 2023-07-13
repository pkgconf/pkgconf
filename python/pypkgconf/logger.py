import logging
import typing as T

logger = logging.getLogger('pkgconf')
formatter = logging.Formatter('%(asctime)s %(levelname)s:%(name)s: %(message)s')


def set_stream_handler(stream):
    logger.handlers.clear()

    handler = logging.StreamHandler(stream)
    handler.setFormatter(formatter)
    logger.addHandler(handler)


def set_file_handler(file):
    logger.handlers.clear()

    handler = logging.FileHandler(file)
    handler.setFormatter(formatter)
    logger.addHandler(handler)
