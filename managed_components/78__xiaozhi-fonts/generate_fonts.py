import os

# 定义要生成的字体配置
font_configs = [
    (14, 1),  # 14号字体，1 bpp
    (16, 4),  # 16号字体，4 bpp
    (20, 4),  # 20号字体，4 bpp
    (30, 4),  # 30号字体，4 bpp
]

def main():
    # 遍历所有字体配置
    for size, bpp in font_configs:
        print(f"\n正在生成 {size}px 字体，{bpp} bpp...")
        
        # 构建命令并执行
        cmd = f"python font.py lvgl --font-size {size} --bpp {bpp}"
        ret = os.system(cmd)
        
        if ret != 0:
            print(f"生成 {size}px 字体失败")
        else:
            print(f"生成 {size}px 字体成功")

if __name__ == "__main__":
    main() 