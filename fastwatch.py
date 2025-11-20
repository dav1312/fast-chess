import sys
import subprocess
import time
import re
import os
import math

from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QGridLayout, QLabel, QSizePolicy, QScrollArea, QFrame)
from PyQt6.QtSvgWidgets import QSvgWidget
from PyQt6.QtCore import QThread, pyqtSignal, Qt, QByteArray

try:
    import chess
    import chess.svg
except ImportError:
    print("Error: 'python-chess' library is required.")
    print("pip install python-chess")
    sys.exit(1)

# Configuration
LOG_FILENAME = "fastchess_gui.log"
COLUMNS = 3  # How many boards per row

class GameWidget(QWidget):
    """
    A widget representing a single game (Thread).
    Contains a Label (Game Info) and an SVG Widget (Board).
    """
    def __init__(self, thread_id):
        super().__init__()
        self.thread_id = thread_id
        
        # Styling
        self.setStyleSheet("background-color: #ffffff; border: 1px solid #cccccc; border-radius: 5px;")
        
        layout = QVBoxLayout()
        layout.setContentsMargins(5, 5, 5, 5)
        
        # Title / Info
        self.info_label = QLabel(f"Game {thread_id}")
        self.info_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.info_label.setStyleSheet("font-weight: bold; border: none; font-size: 12px;")
        layout.addWidget(self.info_label)
        
        # Board SVG
        self.svg_widget = QSvgWidget()
        # Set a minimum size so they don't get crushed in the grid
        self.svg_widget.setMinimumSize(300, 300)
        layout.addWidget(self.svg_widget)
        
        self.setLayout(layout)

    def update_state(self, svg_data, info_text):
        self.svg_widget.load(QByteArray(svg_data.encode('utf-8')))
        self.info_label.setText(info_text)

class LogReader(QThread):
    """
    Background thread to parse the log file.
    Emits signal (thread_id, svg_string, info_string)
    """
    board_updated = pyqtSignal(str, str, str)
    game_finished = pyqtSignal()

    def __init__(self, process):
        super().__init__()
        self.process = process
        self.running = True
        
        # Pattern to extract Thread ID: matches [ 3] or < 3> or <3>
        self.thread_pattern = re.compile(r'[<\[]\s*(\d+)\s*[>\]]')

    def parse_position(self, line):
        if "position" not in line:
            return None

        # Simple logic: parse the 'position ...' part of the line
        # Fastchess logs: [Time] <Thread> Name > position ...
        
        # 1. Extract Position Command
        match_startpos = re.search(r'position\s+startpos(\s+moves\s+(.*))?', line)
        match_fen = re.search(r'position\s+fen\s+(.*?)\s+moves\s+(.*)', line)
        match_fen_only = re.search(r'position\s+fen\s+(.*)', line)

        board = chess.Board()
        try:
            if match_startpos:
                board.reset()
                if match_startpos.group(2):
                    for move in match_startpos.group(2).split():
                        board.push_uci(move)
                return board
            elif match_fen:
                board = chess.Board(match_fen.group(1))
                if match_fen.group(2):
                    for move in match_fen.group(2).split():
                        board.push_uci(move)
                return board
            elif match_fen_only:
                board = chess.Board(match_fen_only.group(1).strip())
                return board
        except ValueError:
            return None
        return None

    def run(self):
        # Wait for log file
        while not os.path.exists(LOG_FILENAME):
            time.sleep(0.1)
            if self.process.poll() is not None:
                return

        with open(LOG_FILENAME, 'r', encoding='utf-8', errors='ignore') as f:
            while self.running:
                line = f.readline()
                if not line:
                    if self.process.poll() is not None:
                        self.game_finished.emit()
                        break
                    time.sleep(0.01) # Low latency poll
                    continue

                # 1. Extract Thread ID
                # We look for the pattern < 3> or [ 3]
                match = self.thread_pattern.search(line)
                if not match:
                    continue
                
                thread_id = match.group(1)

                # 2. Parse Board
                board = self.parse_position(line)
                if board:
                    # Generate Cburnett Style SVG
                    svg = chess.svg.board(
                        board=board,
                        lastmove=board.peek() if board.move_stack else None,
                        size=400 # Render resolution
                    )
                    
                    turn_str = "White" if board.turn else "Black"
                    ply = board.fullmove_number
                    info = f"Thread {thread_id} | Move {ply} ({turn_str})"
                    
                    self.board_updated.emit(thread_id, svg, info)

    def stop(self):
        self.running = False

class MainWindow(QMainWindow):
    def __init__(self, fastchess_process):
        super().__init__()
        self.process = fastchess_process
        self.setWindowTitle("Fastchess Multi-Board Viewer")
        self.resize(1000, 800)
        
        self.boards = {} # Maps thread_id -> GameWidget

        # -- Layout Architecture --
        # Main Window -> Central Widget -> VBox -> [ScrollArea, StatusLabel]
        # ScrollArea -> ContainerWidget -> GridLayout -> GameWidgets

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # Scroll Area for Grid
        self.scroll = QScrollArea()
        self.scroll.setWidgetResizable(True)
        self.scroll.setStyleSheet("background-color: #e0e0e0;")
        
        # Container for the grid
        self.grid_container = QWidget()
        self.grid_layout = QGridLayout(self.grid_container)
        self.grid_layout.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignLeft)
        self.scroll.setWidget(self.grid_container)
        
        main_layout.addWidget(self.scroll)

        # Bottom Status Bar
        self.status_label = QLabel("Initializing...")
        self.status_label.setStyleSheet("padding: 5px; font-size: 14px; background-color: white;")
        main_layout.addWidget(self.status_label)

        # Thread
        self.reader = LogReader(self.process)
        self.reader.board_updated.connect(self.handle_update)
        self.reader.game_finished.connect(self.on_finished)
        self.reader.start()

    def handle_update(self, thread_id, svg, info):
        # If this is a new thread ID we haven't seen, create a board
        if thread_id not in self.boards:
            self.add_board(thread_id)
        
        # Update specific board
        self.boards[thread_id].update_state(svg, info)
        
        # Update main status
        self.status_label.setText(f"Active Games: {len(self.boards)} | Latest update from Thread {thread_id}")

    def add_board(self, thread_id):
        widget = GameWidget(thread_id)
        self.boards[thread_id] = widget
        
        # Calculate Grid Position
        # 0 1 2
        # 3 4 5
        count = len(self.boards) - 1
        row = math.floor(count / COLUMNS)
        col = count % COLUMNS
        
        self.grid_layout.addWidget(widget, row, col)

    def on_finished(self):
        self.status_label.setText("Tournament Finished.")
        self.status_label.setStyleSheet("padding: 5px; font-size: 14px; background-color: #ffcccc;")

    def closeEvent(self, event):
        self.reader.stop()
        self.process.terminate()
        event.accept()

def main():
    if len(sys.argv) < 2:
        print("Usage: python fastwatch.py [fastchess arguments]")
        sys.exit(1)

    user_args = sys.argv[1:]
    filtered_args = [arg for arg in user_args if not arg.startswith("-log")]
    
    cmd = ["./fastchess"] + filtered_args + [
        "-log", f"file={LOG_FILENAME}", "engine=true", "realtime=true"
    ]

    if os.path.exists(LOG_FILENAME):
        try:
            os.remove(LOG_FILENAME)
        except:
            pass

    print(f"Launching: {' '.join(cmd)}")
    try:
        process = subprocess.Popen(cmd)
    except FileNotFoundError:
        print("Error: fastchess executable not found.")
        sys.exit(1)

    app = QApplication(sys.argv)
    window = MainWindow(process)
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()