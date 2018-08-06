import sys
import os
import shlex
from recommonmark.parser import CommonMarkParser
from datetime import datetime
from subprocess import Popen, PIPE

def get_version():
    """
    Returns project version as string from 'git describe' command.
    """
    pipe = Popen('git describe --tags --always', stdout=PIPE, shell=True)
    version = pipe.stdout.read()

    if version:
        return version.rstrip().lstrip('v')
    else:
        return 'X.Y'

extensions = [
    'sphinx.ext.mathjax',
]

templates_path = ['_templates']

source_suffix = ['.rst', '.md']

master_doc = 'index'

project = u'Kiva'
copyright = u'2012-' + str(datetime.now().year) + u', Big Ladder Software'
author = u'Neal Kruis'

version = get_version()
release = version
exclude_patterns = ['_build']
pygments_style = 'sphinx'

language = 'en'

todo_include_todos = False


html_theme = 'sphinx_rtd_theme'

html_static_path = ['_static']
htmlhelp_basename = 'Kivadoc'
#html_split_index = True
#html_theme_options = {'collapsiblesidebar': True}

latex_elements = {}

latex_documents = [
  (master_doc, 'Kiva.tex', u'Kiva Documentation',
   u'Neal Kruis', 'manual'),
]
man_pages = [
    (master_doc, 'kiva', u'Kiva Documentation',
     [author], 1)
]
