# 小米 BE7000 U-Boot

本项目基于 [1980490718/u-boot-2016](https://github.com/1980490718/u-boot-2016) 的 Qualcomm QSDK 12.5 U-Boot，提供小米 BE7000 的构建配置和基于 lwIP 的 Webfailsafe 恢复界面。

| 平台 | 构建目标 | 设备 |
| --- | --- | --- |
| IPQ9574 | `ipq9574_xiaomi_be7000` | 小米 BE7000 |

## 源码范围

`board/` 仅保留 IPQ9574 板级实现和 Qualcomm 共用代码。`arch/arm/dts/` 只构建 BE7000 的设备树，板级 DTS 引用的基础描述作为 DTSI 保留。ARMv7、闪存、网络和 Webfailsafe 共用代码参与构建。

## ART/MAC 定制

本源码已移植 BE7000 本地 11.15 定制版的 ART/MAC 修改，支持六个有线和六个无线 MAC 槽位。无线校准区位于 ART 的 `0x1000`、`0x33000`、`0x65000`，读写和 Webfailsafe 校验使用各校准区对应的 MAC 与 XOR16 字段。

写入通过 NAND MTD 完成，整块读取并保留同一擦除块内的其他 ART 数据，写入后回读核对修改范围。

## Linux 构建

安装依赖（Ubuntu）

```bash
sudo apt-get update
sudo apt-get install -y build-essential libncurses-dev gawk git gettext libssl-dev python3 \
  wget cpio flex bison bc rsync nodejs npm gzip zopfli device-tree-compiler
npm install -g html-minifier-terser@7.2.0 clean-css@5.3.3 terser@5.51.2 svgo@4.1.0
```

将作者的 ARM OpenWrt 工具链克隆到本仓库的同级目录

```bash
git clone https://github.com/lyleee0083/xiaomi-be7000-uboot.git
git clone --depth=1 https://github.com/1980490718/toolchain-arm_cortex-a7_gcc-5.2.0.git staging_dir
cd xiaomi-be7000-uboot
export NODE_PATH="$(npm root -g)"
./build.sh ipq9574_xiaomi_be7000
```

产物为 `bin/openwrt-ipq9574_xiaomi_be7000-u-boot.mbn`，格式为 MBN v6（SHA-384）。
`./build.sh clean_all` 清理 MBN 文件，`./build.sh clean` 同时清理构建中间文件。

GitHub Actions 的 **Build Xiaomi BE7000 U-Boot** 工作流支持 `main` 推送和手动运行，成功后可在该次运行的 Artifacts 下载 MBN。

## 许可与声明

- 本项目使用 GPLv2 开源，详见 LICENSE 文件
- 任何借鉴、复刻、二次开发须注明出处并以 GPLv2 开源，保留原始版权声明
- 本项目未经充分测试，使用风险自负
- 作者不承担因使用本代码而产生的任何损失或损害责任
- 仅供学习和研究使用，禁止用于侵犯他人利益的用途
