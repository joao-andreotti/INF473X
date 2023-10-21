import random
import time

from selenium import webdriver
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options

target_url = 'https://superxwebdeveloper.epizeuxis.net/'

# Obtained from github
my_username = "superXWebDeveloper"
my_password = "bA8wd5NkGvtMdaYgBbD3"

def get_time():
    return int(time.time())

def sync_time():
    init_time = get_time()
    while get_time() == init_time:
        pass

def challenge(id, time):
    random.seed(time + id)
    solution = random.sample(list(range(1,50)), 5)
    solution.sort()
    return solution

def bruteforce_id(time, sol):
    for i in range(1,1000000):
        if challenge(id=i, time=time) == sol:
            return i

def try_sol(sol, driver):
    cmd = ''.join([f'toggle({ball});' for ball in sol])
    driver.execute_script(cmd)
    
    submit = driver.find_element(By.CLASS_NAME, 'validate')
    time_at_click = get_time()
    submit.click()

    return time_at_click

def main():
    # Creates driver
    chrome_options = Options()
    chrome_options.add_experimental_option("detach", True)

    driver = webdriver.Chrome(chrome_options=chrome_options)

    # Logs in
    driver.get(target_url)

    uname = driver.find_element(By.NAME, "username")
    uname.clear()
    uname.send_keys(my_username)

    pwd = driver.find_element(By.NAME, "password")
    pwd.clear()
    pwd.send_keys(my_password)

    submit = driver.find_element(By.CLASS_NAME, 'validate')
    submit.click()

    print(driver.get_cookie("session"))

    # Gets desired solution for a sample query
    sol = [1, 2, 3, 4, 5]
    sync_time()
    curr_time = try_sol(sol, driver)

    sol_txt = driver.find_element(By.TAG_NAME, 'h3').text
    sol_balls = sol_txt[sol_txt.find('[') + 1: sol_txt.find(']')].split(',')
    curr_sol = [int(ball) for ball in sol_balls]

    # Bruteforces ID locally
    id = bruteforce_id(time=curr_time, sol=curr_sol)
    print(id)
    
    # Tries predicted solution
    sync_time()
    sol = challenge(id=id, time=get_time())
    try_sol(sol=sol, driver=driver)

    # Prints signed cookie
    # print(driver.get_cookie("session"))

if __name__=='__main__':
    print(challenge(0, 5))
    main()