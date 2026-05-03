import requests
import json

api_key = "f206e14d-7966-450c-b18c-c68fe0bcdce8"

def get_uuid(name):
    url = f"https://api.mojang.com/users/profiles/minecraft/{name}"
    resp = requests.get(url)
    if resp.status_code == 200:
        return resp.json().get("id")
    return None

def test_hypixel_api(name):
    uuid = get_uuid(name)
    if not uuid:
        print(f"UUID not found for {name}")
        return

    url = f"https://api.hypixel.net/v2/player?key={api_key}&uuid={uuid}"
    print(f"Fetching: {url}")
    
    response = requests.get(url)
    if response.status_code != 200:
        print(f"Error: {response.status_code}")
        print(response.text)
        return

    data = response.json()
    if not data.get("success"):
        print("API Error:", data.get("cause"))
        return

    player = data.get("player")
    if not player:
        print("Player not found in Hypixel (maybe never played?)")
        return

    # Extract Stats
    stats = player.get("stats", {}).get("Bedwars", {})
    achievements = player.get("achievements", {})
    
    bw_level = achievements.get("bedwars_level", 0)
    final_kills = stats.get("final_kills_bedwars", 0)
    final_deaths = stats.get("final_deaths_bedwars", 0)
    fkdr = final_kills / final_deaths if final_deaths > 0 else final_kills
    
    network_exp = player.get("networkExp", 0)
    # Correct formula: level = (sqrt(2 * networkExp + 15312.5) - 125) / 50 + 1
    # Simplified: (network_exp * 2 + 15312.5)**0.5 / 50 - 2.5 + 1? No.
    # Level 1 is 0 exp.
    # Level 2 is 10000 exp.
    # Level 3 is 21000 exp.
    level = (network_exp * 2 + 15312.5)**0.5 / 50 - 2.5
    
    print(f"Player: {player.get('displayname')}")
    print(f"Hypixel Level: {level:.2f}")
    print(f"Bedwars Star: {bw_level}")
    print(f"Final Kills: {final_kills}")
    print(f"FKDR: {fkdr:.2f}")

if __name__ == "__main__":
    test_hypixel_api("Technoblade")
    test_hypixel_api("Hypixel")
