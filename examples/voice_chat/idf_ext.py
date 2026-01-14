from pathlib import Path
import sys

EXAMPLES_DIR = Path(__file__).resolve().parent.parent
sys.path.append(str(EXAMPLES_DIR))

from common.boards.idf_ext_boards import action_extensions

# The implementation of this file is common and is present in the common board directory. Nothing to do here.
