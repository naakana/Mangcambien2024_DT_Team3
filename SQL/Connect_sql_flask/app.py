from flask import Flask, jsonify, render_template, request
from flask_mysqldb import MySQL

app = Flask(__name__)
mysql = MySQL(app)

app.config['MYSQL_DATABASE_USER'] = 'root'
app.config['MYSQL_DATABASE_PASSWORD'] = ''
app.config['MYSQL_DATABASE_DB'] = 'doan'
app.config['MYSQL_DATABASE_HOST'] = 'localhost'

@app.route("/receive_data", methods=['POST'])
def receive_data():
    data = request.get_json()

    if 'id' not in data or 'temperature' not in data or 'humidity' not in data:
        return jsonify({"error": "Dữ liệu không hợp lệ"}), 400

    id = data['id']
    temperature = data['temperature']
    humidity = data['humidity']

    try:
        cur = mysql.connection.cursor()
        cur.execute("USE doan")
        cur.execute("""
            UPDATE quanli
            SET temperature = %s, humidity = %s
            WHERE id = %s
        """, (temperature, humidity, id))

        cur.execute("""
            INSERT INTO history (temperature, humidity)
            VALUES (%s, %s)
        """, (temperature, humidity))

        if cur.rowcount == 0:
            return jsonify({"error": "Không tìm thấy ID để cập nhật."}), 404

        mysql.connection.commit()
        cur.close()

        return jsonify({"message": "Cập nhật thành công"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route("/send_data", methods=['POST'])
def send_data():
    print("Received request")
    data = request.get_json()
    
    if 'id' not in data:
        return jsonify({"error": "Dữ liệu không hợp lệ, cần ID."}), 400
    
    id = data['id']
    print(f"Received ID: {id}")  
    con = mysql.connection
    cur = con.cursor()
    cur.execute("USE doan")
    try:
        cur.execute("SELECT * FROM quanli WHERE id = %s", (id,))
        sensor_nodes = cur.fetchall()  
        print(f"Sensor nodes: {sensor_nodes}") 
        if not sensor_nodes:
            return jsonify({"error": "Không tìm thấy node nào cho ID."}), 404

        node_list = []
        for node in sensor_nodes:
            node_data = { 
                'led': node[3],  
            }
            node_list.append(node_data)

        return jsonify({'nodes': node_list})  
    except Exception as e:
        print(f"Error: {str(e)}")  
        return jsonify({"error": "Đã xảy ra lỗi trong máy chủ"}), 500


@app.route("/", methods=['GET', 'POST'])
def user_dashboard():
    con = mysql.connection
    cur = con.cursor()
    cur.execute("USE doan")
    
    if request.method == 'POST':
        if 'led_control' in request.form:
            node_id = request.form['led_control']  
            cur.execute("UPDATE quanli SET led = NOT led WHERE id = %s", (node_id,))
            mysql.connection.commit()  

    cur.execute("SELECT id, temperature, humidity, led FROM quanli ORDER BY id")
    quanli = cur.fetchall()

    cur.execute("SELECT * FROM history WHERE id = 1")
    history = cur.fetchall()
    
    cur.close() 
    return render_template('user.html', quanli=quanli, history=history)


if __name__ == "__main__":
    app.run(host='0.0.0.0', debug=True, port=5000)
