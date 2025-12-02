import json
import sys

def extract_data(data):
    result = []
    for element in data:
        sv = element['sv']
        x_b, y_b, v_b, a_b = sv['x'], sv['y'], sv['velX'], sv['accelX']
        for pov in element['povs']:
            agent = pov['agent']
            if agent['y'] > y_b and abs(agent['x'] - x_b) < 2:
                x_f, y_f, v_f, a_f = agent['x'], agent['y'], agent['velX'], agent['accelX']
                result.append(f"{x_b} {y_b} {v_b} {a_b} {x_f} {y_f} {v_f} {a_f}")
                break
    return result

if __name__ == '__main__':
    data = json.load(sys.stdin)
    result = extract_data(data)
    for r in result:
        print(r)
