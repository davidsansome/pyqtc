import argparse
import collections
import os
import re
import subprocess
import sys
import xml.etree.cElementTree as ElementTree

VERSION     = "2.7.2"
NAMESPACE   = "pyqtc.python.%s" % VERSION
FILTER_NAME = "Python %s" % VERSION
FILTER_ID   = "python"

EXTENSIONS  = {".html", ".css", ".js", ".txt", ".xml", ".jpg"}


class Symbol(object):
  def __init__(self, full_name, type, filename):
    self.full_name = full_name
    self.type = type
    self.filename = filename
    self.name = full_name.split(".")[-1]


def LoadSphinxIndex(filename):
  ret = []

  with open(filename) as handle:
    for line in handle:
      if not line or line.startswith("#"):
        continue

      parts = line.strip().split(" ")
      ret.append(Symbol(full_name=parts[0], type=parts[1],
        filename="%s#%s" % (parts[2], parts[0])))
  
  return ret


class Element(object):
  def __init__(self, builder, name, args=None):
    if args is None:
      args = {}
    
    self.builder = builder
    self.name = name
    self.args = args
  
  def __enter__(self):
    self.builder.start(self.name, self.args)
  
  def __exit__(self, _exc_type, _exc_value, _traceback):
    self.builder.end(self.name)


def Data(builder, element_name, data=None, args=None):
  if args is None:
    args = {}
  builder.start(element_name, args)
  if data is not None:
    builder.data(data)
  builder.end(element_name)


def WriteQhp(symbols, files, qhp_filename):
  builder = ElementTree.TreeBuilder()

  with Element(builder, "QtHelpProject", {"version": "1.0"}):
    Data(builder, "namespace", NAMESPACE)
    Data(builder, "virtualFolder", "doc")

    with Element(builder, "customFilter", {"name": FILTER_NAME}):
      Data(builder, "filterAttribute", FILTER_ID)
      Data(builder, "filterAttribute", VERSION)

    with Element(builder, "filterSection"):
      Data(builder, "filterAttribute", FILTER_ID)
      Data(builder, "filterAttribute", VERSION)

      with Element(builder, "toc"):
        pass
      
      with Element(builder, "keywords"):
        for sym in symbols:
          Data(builder, "keyword", args={
            "name": sym.full_name,
            "id": sym.full_name,
            "ref": sym.filename
          })
      
      with Element(builder, "files"):
        for filename in files:
          Data(builder, "file", filename)
  
  with open(qhp_filename, "w") as handle:
    handle.write(ElementTree.tostring(builder.close()))


def WriteQhcp(qhp_filenames, qch_filenames, qhcp_filename):
  builder = ElementTree.TreeBuilder()

  with Element(builder, "QHelpCollectionProject", {"version": "1.0"}):
    with Element(builder, "docFiles"):
      with Element(builder, "generate"):
        for i, filename in enumerate(qhp_filenames):
          with Element(builder, "file"):
            Data(builder, "input", filename)
            Data(builder, "output", qch_filenames[i])
      
      with Element(builder, "register"):
        for filename in qch_filenames:
          Data(builder, "file", filename)
  
  with open(qhcp_filename, "w") as handle:
    handle.write(ElementTree.tostring(builder.close()))


def GetFileList(path):
  ret = []
  for root, _dirnames, filenames in os.walk(path):
    for filename in filenames:
      if os.path.splitext(filename)[1] in EXTENSIONS:
        ret.append(os.path.relpath(os.path.join(root, filename), path))
  return ret


def AdjustSpinxConf(filename):
  contents = open(filename).read()
  contents += '\nhtml_theme="sphinx-theme"' \
              '\nhtml_theme_path=["%s"]\n' % \
                  os.path.join(os.path.dirname(__file__))

  contents = re.sub(r'html_use_opensearch .*', '', contents)

  open(filename, 'w').write(contents)


def main(args):
  parser = argparse.ArgumentParser(
    description="Builds a Qt Help file from Sphinx documentation")
  
  parser.add_argument("--sphinx-dir", required=True, help="directory containing objects.inv")
  parser.add_argument("--qhp", required=True, help=".qhp output filename")
  parser.add_argument("--qhcp", required=True, help=".qhcp output filename")

  args = parser.parse_args(args)
  qhp = [args.qhp]
  qch = [os.path.splitext(x)[0] + ".qch" for x in qhp]
  qhc = os.path.splitext(args.qhcp)[0] + ".qhc"

  # Edit the conf.py to use our minimal theme
  conf_py = os.path.join(args.sphinx_dir, "conf.py")
  AdjustSpinxConf(conf_py)

  # Build the docs
  subprocess.check_call(["make", "html"], cwd=args.sphinx_dir)

  sphinx_output = os.path.join(args.sphinx_dir, "build/html")

  # Read symbols from the objects.inv
  symbols = LoadSphinxIndex(os.path.join(sphinx_output, "objects.inv"))

  # Get the list of files to include
  files = GetFileList(sphinx_output)

  # Create the output files
  for filename in qhp:
    WriteQhp(symbols, files, filename)
  WriteQhcp(qhp, qch, args.qhcp)

  print "Now run:"
  print "  qcollectiongenerator %s -o %s" % (args.qhcp, qhc)
  print "  assistant-qt4 -collectionFile %s" % qhc


if __name__ == "__main__":
  main(sys.argv[1:])
      