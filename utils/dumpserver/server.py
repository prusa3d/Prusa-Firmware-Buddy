import json
import os
from datetime import datetime, timezone
from time import gmtime, strftime
from flask import Flask, request
from pathlib import Path

app = Flask('dumpserver')
dump_dir = Path(os.environ.get('DUMPDIR', '.'))

dump_dir.mkdir(exist_ok=True, parents=True)


@app.route('/', methods=['POST'])
def post_dump():
    timestamp = strftime("%Y-%m-%d_%H-%M-%S", gmtime())
    printer_type = request.args.get('printer_type')
    board = request.args.get('board')

    filenamebase = f'{timestamp}_{printer_type}_{board}'
    dump_filepath = dump_dir / Path(filenamebase + '_dump.bin')
    with open(dump_filepath, 'wb') as f:
        f.write(request.data)

    metadata_filepath = dump_dir / Path(filenamebase + '_metadata.json')
    metadata = request.args.to_dict()
    metadata['timestamp'] = datetime.now(timezone.utc).timestamp() * 1000
    with open(metadata_filepath, 'w') as f:
        f.write(json.dumps(metadata))

    return '', 204
