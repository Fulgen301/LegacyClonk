#! /usr/bin/env python3

import collections.abc
import datetime
import json
import sys
import tomllib
import typing


TomlValue = typing.Union[dict, str, int, float, bool,
                         datetime.datetime, datetime.date, datetime.time, list]


def transform_matrix(matrix: dict) -> list:
    new_matrix = []
    for os, runner in matrix.items():
        for config in runner["config"]:
            new_matrix.append({
                "runner": runner["runner"],
                "os": os.capitalize(),
                **config
            })
    return new_matrix


def print_output(toml: dict, name: str, transform: collections.abc.Callable[[TomlValue], TomlValue] = lambda x: x):
    print(f"{name}={json.dumps(transform(toml[name]))}")


if __name__ == "__main__":
    toml = tomllib.load(open(sys.argv[1], "rb"))
    print_output(toml, "matrix", transform_matrix)
