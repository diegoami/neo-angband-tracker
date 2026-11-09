#!/usr/bin/env python3
"""Read-only desktop browser for recorded native C and Neo Borg evidence."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent / 'browser'))
from server import main
if __name__ == '__main__':
    main()
