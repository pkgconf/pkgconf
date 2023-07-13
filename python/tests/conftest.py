import pytest

import os
from pathlib import Path


@pytest.fixture
def testsdir() -> Path:
    return Path(__file__).parents[2] / 'tests'


@pytest.fixture
def lib1_pkg_config_path(monkeypatch, testsdir):
    monkeypatch.setenv('PKG_CONFIG_PATH', str(testsdir / 'lib1'))
    yield


@pytest.fixture
def lib2_pkg_config_path(monkeypatch, testsdir):
    monkeypatch.setenv('PKG_CONFIG_PATH', str(testsdir / 'lib2'))
    yield


@pytest.fixture
def lib1lib2_pkg_config_path(monkeypatch, testsdir):
    paths = [
        str(testsdir / 'lib1'),
        str(testsdir / 'lib2'),
    ]

    monkeypatch.setenv('PKG_CONFIG_PATH', os.pathsep.join(paths))
    yield
