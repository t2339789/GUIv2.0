import urllib.request
import json

def test_name():
    api_key = "f206e14d-7966-450c-b18c-c68fe0bcdce8"
    name = "Technoblade"
    url = f"https://api.hypixel.net/v2/player?key={api_key}&name={name}"
    
    try:
        with urllib.request.urlopen(url) as response:
            data = json.loads(response.read().decode())
            if data.get('success'):
                player = data.get('player')
                if player:
                    print(f"Success! Found player {player.get('displayname')}")
                    return True
                else:
                    print("Success, but player is null")
            else:
                print(f"API Error: {data.get('cause')}")
    except Exception as e:
        print(f"Request failed: {e}")
    return False

test_name()
