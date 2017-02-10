#!/usr/bin/env python

import argparse
import subprocess


def connect(args):
    host = '{}:{}'.format(args.ip, args.port)
    subprocess.call(['adb', 'connect', host])


def upload(args):
    subprocess.call(['adb', 'push', args.bin + args.program, args.path + args.program])


def run(args):
    execute = """
        adb shell kk
        adb shell killall -9 {1}
        adb shell '(cd {0} && ./{1})'
    """.format(args.path, args.program)

    subprocess.call(execute, shell=True)


if __name__ == '__main__':
    PROGRAM_PATH = "/data/ftp/internal_000/tudelft_vision/"

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    parser_connect = subparsers.add_parser('connect')
    parser_connect.add_argument('--ip', type=str, default="192.168.42.1")
    parser_connect.add_argument('--port', type=int, default=9050)
    parser_connect.set_defaults(func=connect)

    parser_upload = subparsers.add_parser('upload')
    parser_upload.add_argument('program', type=str)
    parser_upload.add_argument('--bin', type=str, default="build/bin/")
    parser_upload.add_argument('--path', type=str, default=PROGRAM_PATH)
    parser_upload.set_defaults(func=upload)

    parser_run = subparsers.add_parser('run')
    parser_run.add_argument('program', type=str)
    parser_run.add_argument('--path', type=str, default=PROGRAM_PATH)
    parser_run.set_defaults(func=run)

    args = parser.parse_args()
    args.func(args)
