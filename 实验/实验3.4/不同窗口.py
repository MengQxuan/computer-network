# import matplotlib.pyplot as plt
# import pandas as pd

# # 示例数据
# data = {
#     'Delay_Percent': ['0%', '0%', '0%', '1%', '1%', '1%', '5%', '5%', '5%', '10%', '10%', '10%'],
#     'Transmission_Delay_s': [31,26,20,44,37,31,74,68,59,128,117,132],
#     'Window_Size': [4, 8, 16, 4, 8, 16, 4, 8, 16, 4, 8, 16],
#     'Throughput_Byte_s': [
#         53909,64680,80850,35933,46196,53912,23108,24798,26952,12415,14757,12628
#     ]
# }

# # 创建 DataFrame
# df = pd.DataFrame(data)

# plt.rcParams['font.family'] = ['SimHei']  # 替换为你系统中存在的中文字体
# plt.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题

# # 设置绘图风格为 Matplotlib 内置样式，例如 'ggplot'
# plt.style.use('ggplot')

# # 创建图形和轴
# fig, ax = plt.subplots(figsize=(10, 6))

# # 定义窗口大小的颜色
# colors = {4: 'blue', 8: 'green', 16: 'red'}

# # 获取所有的窗口大小
# window_sizes = sorted(df['Window_Size'].unique())

# # 为每个窗口大小绘制不同的线
# for ws in window_sizes:
#     subset = df[df['Window_Size'] == ws]
#     ax.plot(
#         subset['Transmission_Delay_s'],
#         subset['Throughput_Byte_s'],
#         label=f'{ws}',
#         color=colors[ws],
#         marker='o'
#     )

# # 添加标题和标签
# ax.set_title('不同窗口大小(helloworld.txt)', fontsize=16)
# ax.set_xlabel('传输时延 (s)', fontsize=14)
# ax.set_ylabel('吞吐率 (byte/s)', fontsize=14)

# # 添加图例
# ax.legend(title='窗口大小')

# # 显示网格
# ax.grid(True, which='both', linestyle='--', linewidth=0.5)

# # 显示图表
# plt.tight_layout()
# plt.show()


import matplotlib.pyplot as plt
import pandas as pd

# 示例数据
data = {
    'Delay_Percent': ['0%', '0%',  '1%', '1%', '5%', '5%',  '10%', '10%'],
    'Transmission_Delay_s': [173,194,263,316,325,489,627,611],
    'Window_Size': [1,2,1,2,1,2,1,2],
    'Throughput_Byte_s': [
        61061,58445,54323,36178,36827,23904,19046,19183
    ]
}

# 创建 DataFrame
df = pd.DataFrame(data)

plt.rcParams['font.family'] = ['SimHei']  # 替换为你系统中存在的中文字体
plt.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题

# 设置绘图风格为 Matplotlib 内置样式，例如 'ggplot'
plt.style.use('ggplot')

# 创建图形和轴
fig, ax = plt.subplots(figsize=(10, 6))

# 定义窗口大小的颜色
colors = { 1: 'green', 2: 'red'}

# 获取所有的窗口大小
window_sizes = sorted(df['Window_Size'].unique())

# 为每个窗口大小绘制不同的线
for ws in window_sizes:
    subset = df[df['Window_Size'] == ws]
    ax.plot(
        subset['Transmission_Delay_s'],
        subset['Throughput_Byte_s'],
        label=f'{ws}',
        color=colors[ws],
        marker='o'
    )

# 添加标题和标签
ax.set_title('有无拥塞控制(3.jpg)', fontsize=16)
ax.set_xlabel('传输时延 (s)', fontsize=14)
ax.set_ylabel('吞吐率 (byte/s)', fontsize=14)

# 添加图例
ax.legend(title='有拥塞控制1/无拥塞控制2')

# 显示网格
ax.grid(True, which='both', linestyle='--', linewidth=0.5)

# 显示图表
plt.tight_layout()
plt.show()
