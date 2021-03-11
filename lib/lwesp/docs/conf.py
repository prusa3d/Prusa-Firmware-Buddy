# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))
from sphinx.builders.html import StandaloneHTMLBuilder
import subprocess, os

# Run doxygen first
# read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'
# if read_the_docs_build:
subprocess.call('doxygen doxyfile.doxy', shell=True)
# -- Project information -----------------------------------------------------

project = 'LwESP'
copyright = '2020, Tilen MAJERLE'
author = 'Tilen MAJERLE'

# The full version, including alpha/beta/rc tags
version = '1.0.0'

# Try to get branch at which this is running
# and try to determine which version to display in sphinx
git_branch = ''
res = os.popen('git branch').read().strip()
for line in res.split("\n"):
    if line[0] == '*':
        git_branch = line[1:].strip()

# Decision for display version
try:
    if git_branch.index('develop') >= 0:
        version = "latest-develop"
except Exception:
    print("Exception for index check")

# For debugging purpose
print("GIT BRANCH: " + git_branch)
print("VERSION: " + version)

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.autosectionlabel',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.mathjax',
    'sphinx.ext.ifconfig',
    'sphinx.ext.viewcode',
    'sphinx_sitemap',

    'breathe',
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

highlight_language = 'c'

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'canonical_url': '',
    'analytics_id': '',  #  Provided by Google in your dashboard
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,

    'logo_only': False,

    # Toc options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}
html_logo = 'static/images/logo.svg'
github_url = 'https://github.com/MaJerle/esp-at-lib'
html_baseurl = 'https://docs.majerle.eu/projects/esp-at-lib/'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['static']
html_css_files = [
    'css/common.css',
    'css/custom.css',
]
html_js_files = [
    'https://kit.fontawesome.com/3102794088.js'
]

master_doc = 'index'

#
# Breathe configuration
#
#
#
breathe_projects = {
	"esp_at_lib": "_build/xml/"
}
breathe_default_project = "esp_at_lib"
breathe_default_members = ('members', 'undoc-members')