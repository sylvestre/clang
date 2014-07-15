#!/usr/bin/env python
# -*- coding: utf-8 -*-

from flask import Flask, url_for, request, render_template
from flask_bootstrap import Bootstrap

import os
import sys
core_path = os.path.abspath('../core')
sys.path.append(core_path)

import Operations

def main():
	app = Flask(__name__)
	Bootstrap(app)

	app.config['BOOTSTRAP_SERVE_LOCAL'] = True

	@app.route('/')
	def show_launches():
	    logCommand = Operations.BaseLogCommandExecutor( containing_dir = '../core/test/project_v1')
	    return render_template('launches.html', launches=logCommand.execute())

	app.run( debug = True )

if __name__ == '__main__':
	main()