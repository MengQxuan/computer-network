from flask import Flask, render_template

app = Flask(__name__)

# 首页路由，渲染index.html模板
@app.route('/')
def home():
    return render_template('index.html')

if __name__ == '__main__':
    # 运行Flask开发服务器
    app.run(host='0.0.0.0', port=5000, debug=True)
