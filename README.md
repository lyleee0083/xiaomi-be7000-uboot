# xiaomi-be7000-uboot

面向小米 BE7000（Qualcomm IPQ9574）的 U-Boot 项目，基于 [1980490718/u-boot-2016](https://github.com/1980490718/u-boot-2016) 的 Qualcomm QSDK 12.5 U-Boot。项目提供 `ipq9574_xiaomi_be7000` 构建目标、基于 lwIP 的 Webfailsafe 恢复界面，以及针对原厂 ART 分区布局的 MAC 读写与校验。

## 功能

Webfailsafe 提供网页终端、分区备份和刷写。ART/MAC 定制支持原厂布局下的 MAC 读写与校验。

### ART/MAC 定制

ART 布局来自小米 BE7000 原厂固件。项目支持六个有线和六个无线 MAC 槽位。无线校准区位于 ART 的 `0x1000`、`0x33000` 和 `0x65000`，读写和 Webfailsafe 校验使用各校准区对应的 MAC 与 XOR16 字段。

写入通过 NAND MTD 完成。程序整块读取，保留同一擦除块内的其他 ART 数据，写入后逐字节回读核对整个擦除块。读写范围受 SMEM 报告的实际分区长度和 NAND 几何约束。

## 快速开始

先在 [Releases](https://github.com/lyleee0083/xiaomi-be7000-uboot/releases) 选择适用于 BE7000 的 MBN 镜像并核对 SHA-256。使用 Webfailsafe 前，请让电脑与路由器直连，并与其他设备隔离。

### Webfailsafe 备份与刷写

Webfailsafe 通过 HTTP 提供上游网页终端、分区备份和刷写。网页终端无需登录。关闭自动重启后，可以在网页上手动重启。

NAND 数据区分块备份用 `FF` 字节填充坏块，保留其物理位置。需要保留 OOB 数据时，请选择 OOB 模式。

### ART 恢复

恢复前请保留当前设备的独立备份。恢复使用与设备 ART 分区等长、通过 BE7000 无线校准校验的完整镜像。程序在擦除前检查所有物理块并暂存可读的原始数据，随后逐块写入并回读。

若写入失败，串口会报告偏移和失败阶段，程序会尝试回滚已触及的可读块。若日志显示回滚不完整，请保持供电并使用设备专属备份恢复。断电、坏块或不可纠正的旧数据会限制回滚。

## 构建与测试

1. 打开 [Build Xiaomi BE7000 U-Boot](https://github.com/lyleee0083/xiaomi-be7000-uboot/actions/workflows/build-ipq-uboot.yml)。
2. 点击 **Run workflow**，选择 `main`，然后启动构建。
3. 构建成功后打开该次运行，在 **Artifacts** 下载 `openwrt-ipq9574_xiaomi_be7000-u-boot-<版本>.mbn`。

`main` 分支推送和以 `main` 为目标的 Pull Request 也会触发工作流。工作流检查 Webfailsafe 传输、NAND 备份和 ART 回滚，并使用 Python 3 封装 MBN v6（SHA-384）镜像。Actions 产物保留 30 天。

## 仓库结构

| 路径 | 用途 |
| --- | --- |
| [`board/`](board) | IPQ9574 板级实现和 Qualcomm 共用代码 |
| [`arch/arm/dts/`](arch/arm/dts) | BE7000 设备树及其引用的基础 DTSI |
| [`failsafe/`](failsafe) | Webfailsafe 源码及网页资源 |
| [`configs/`](configs)、[`scripts/build-be7000.sh`](scripts/build-be7000.sh) | 构建配置及构建入口 |
| [`tests/`](tests)、[`.github/workflows/`](.github/workflows) | 离线测试与 GitHub Actions 工作流 |

`lwip/`、`main/`、`uip/` 前缀的标签和其他日期标签保留了上游历史快照。

## 版本发布

在 [Releases](https://github.com/lyleee0083/xiaomi-be7000-uboot/releases) 下载 BE7000 的 MBN 镜像，并核对发布页提供的 SHA-256。BE7000 专版使用 `be7000-v` 标签。预发行版本的验证范围见对应发布说明，真机启动和 NAND 擦写需另行验收。

产物版本由 [工作流配置](.github/workflows/build-ipq-uboot.yml) 中的 `ARTIFACT_VERSION` 指定。准备发布新版产物时递增版本号，同一版本的重跑沿用原版本号。

## 许可与声明

- 本项目使用 GPLv2 开源，详见 [LICENSE](LICENSE)。
- 借鉴、复刻或二次开发须注明出处、保留原始版权声明并按 GPLv2 开源。
- 本项目未经充分测试，使用风险自负。作者不承担因使用本代码而产生的损失或损害责任。
- 本项目仅供学习和研究使用，禁止用于侵犯他人利益的用途。
