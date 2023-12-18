import socket
import time 
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.common.by import By

MESSAGE_BUFFER_SIZE = 1024
SERVER_PORT = 12345
SERVER_IP = "127.0.0.1"

def get_square_index(square):
    class_name = square.get_attribute('class')
    return (int(class_name[-1]) - 1) * 8 + (8 - int(class_name[-2]))

def square_to_index(square):
    file = 7 - (ord(square[0]) - ord('a'))
    rank = int(square[1]) - 1
    return rank * 8 + file

def convert_square_index_engine_to_chess(square_index):
    engine_file, engine_rank = square_index % 8, square_index // 8
    chess_file, chess_rank = 7 - engine_file + 1, engine_rank + 1
    return "square-" + str(chess_file) + str(chess_rank)

def await_enemy_move(driver):
    # wait for enemy to make move
    clock = driver.find_element(By.CLASS_NAME, 'clock-bottom')
    clock_status = clock.get_attribute('class')
    while not 'clock-player-turn' in clock_status:
        clock_status = clock.get_attribute('class')

    print("Enemy has made move")
    
    # get enemy move
    highlighted_elements = driver.find_elements(By.CLASS_NAME, 'highlight')

    # if highlighted_elements is empty, then return None
    if len(highlighted_elements) == 0:
        print("No highlighted elements found")
        return None
    
    # insurance
    highlighted_elements = [highlighted_elements[-2], highlighted_elements[-1]]
    first_element_class = highlighted_elements[0].get_attribute('class')
    squares = driver.find_elements(By.CLASS_NAME, first_element_class.split(' ')[-1])
    squares = [square for square in squares if 'piece' in square.get_attribute('class')]
    if len(squares) == 0:
        toElement = highlighted_elements[1]
        fromElement = highlighted_elements[0]
    else:
        toElement = highlighted_elements[0]
        fromElement = highlighted_elements[1]

    toIndex = get_square_index(toElement)
    fromIndex = get_square_index(fromElement)

    # check if its a promotion
    square_name = toElement.get_attribute('class').split(' ')[-1]

    # find by class names 'piece' and 'square'
    piece_squares = driver.find_elements(By.CLASS_NAME, square_name)
    piece_square = None
    for square in piece_squares:
        if 'piece' in square.get_attribute('class'):
            piece_square = square
            break
    piece_square_class_name = piece_square.get_attribute('class')
    last_part = piece_square_class_name.split(' ')[-1]
    if len(last_part) == 2:
        # promotion
        return "{},{},{}".format(fromIndex, toIndex, last_part[1])

    return "{},{},{}".format(fromIndex, toIndex, 'NONE')

def make_move(driver, move_from, move_to, promotion, color):
    # find the game board   
    game_board = driver.find_element(By.CLASS_NAME, 'board')

    # convert engine squares to chess squares and click on them
    from_square = game_board.find_element(By.CLASS_NAME, convert_square_index_engine_to_chess(move_from))
    from_square.click()
    to_square = game_board.find_element(By.CLASS_NAME, convert_square_index_engine_to_chess(move_to))

    # click on the location of to_square
    action = webdriver.common.action_chains.ActionChains(driver)
    action.move_to_element_with_offset(to_square, 0, 0)
    action.click()
    action.perform()

    # if promotion, click on the promotion piece
    if promotion is not None:
        promo_window = game_board.find_element(By.CLASS_NAME, 'promotion-window')
        promo_string = ('w' if color == 0 else 'b') + promotion
        promo_piece = promo_window.find_element(By.CLASS_NAME, promo_string)
        promo_piece.click()

    # sleep for 50 ms
    time.sleep(0.05)
        
def receive_ok(client_socket):
    # receive ok
    data = client_socket.recv(MESSAGE_BUFFER_SIZE)
    while data.decode() != 'ok':
        data = client_socket.recv(MESSAGE_BUFFER_SIZE)

def main():
    # Set up ChromeDriver 
    chrome_driver_path = 'chromedriver.exe'
    options = webdriver.ChromeOptions()
    options.add_experimental_option('excludeSwitches', ['enable-logging'])
    driver = webdriver.Chrome(service=Service(chrome_driver_path), options=options)

    # Create a socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Bind the socket to the server address
    server_socket.bind((SERVER_IP, SERVER_PORT))

    # Listen for incoming connections
    server_socket.listen(1)
    print("Server listening on {}:{}".format(SERVER_IP, SERVER_PORT))

    # Accept a client connection
    client_socket, client_address = server_socket.accept()
    print("Client connected:", client_address)
    
    # login
    driver.get('https://www.chess.com/login')
    driver.find_element(By.ID, 'username').send_keys('ImABonoboXD')
    driver.find_element(By.ID, 'password').send_keys('Wi?4tkeaBWbGYF2') # Wi?4tkeaBWbGYF2
    driver.find_element(By.ID, 'login').click()

    # go to live chess and start a game
    driver.get('https://www.chess.com/live')
    #driver.find_element(By.CSS_SELECTOR, '[data-cy="new-game-index-play"]').click()

    x = input("Press enter when ready to start game")

    # find the game board   
    game_board = driver.find_element(By.CLASS_NAME, 'board')

    # see what color we are
    board_status = game_board.get_attribute('class')
    if 'flipped' in board_status:
        color = 1
    else:
        color = 0

    print(board_status)

    # keep track of move number
    moves = 0

    try:
        while True:
            # await enemy move
            client_socket.send("ponder".encode())
            enemy_move = await_enemy_move(driver)
            print("Enemy move:", enemy_move)
            client_socket.send("stop".encode())
            receive_ok(client_socket)
            if enemy_move is not None:
                # send enemy move to client
                print("Sending enemy move:", enemy_move)
                client_socket.send(enemy_move.encode())
                receive_ok(client_socket)
                moves += 1
            # tell client to search for move
            print("Telling client to search for move")

            # get time left
            span_element = driver.find_element(By.CSS_SELECTOR, "div.clock-bottom span")

            # get time from span element
            time_string = span_element.get_attribute('innerHTML')
            time_string = time_string.split(':')
            time_seconds = int(time_string[0]) * 60 + int(time_string[1])

            # convert to milliseconds
            time_seconds *= 1000
            time_limit = min(10_000, int(time_seconds / max(20, 45 - moves)))

            client_socket.send("start {}L".format(time_limit).encode())

            # Receive data from the client for TIME_LIMIT seconds
            message = None
            while (True):
                new_message = client_socket.recv(MESSAGE_BUFFER_SIZE).decode()
                if new_message == "stop":
                    break
                if new_message:
                    message = new_message
                print("Received message:", message)

            # make move
            split_message = message.strip().split(' ')
            string_move = split_message[-1]
            move_from, move_to = square_to_index(string_move[:2]), square_to_index(string_move[2:4])
            promotion = None if len(string_move) == 4 else string_move[4]
            make_move(driver, move_from, move_to, promotion, color)
            moves += 1
    except Exception as e:
        print(e)
        client_socket.close()
        server_socket.close()
        driver.quit()
        return

if __name__ == '__main__':
    main()