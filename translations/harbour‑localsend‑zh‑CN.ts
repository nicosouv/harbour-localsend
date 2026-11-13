<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>AboutPage</name>
    <message>
        <source>About</source>
        <translation>关于</translation>
    </message>
    <message>
        <source>An unofficial LocalSend client for Sailfish OS. Files go straight from one device to the other over your own network — no account, no server, no Internet connection needed.</source>
        <translation>这是 Sailfish OS 的非官方 LocalSend 客户端。文件在你自己的网络中直接设备互传，无需账号、无需服务器、不需要互联网连接。</translation>
    </message>
    <message>
        <source>Encrypted with a stored key</source>
        <translation>使用已保存密钥加密</translation>
    </message>
    <message>
        <source>Fingerprint</source>
        <translation>指纹</translation>
    </message>
    <message>
        <source>Good to know</source>
        <translation>注意事项</translation>
    </message>
    <message>
        <source>HTTP (not encrypted)</source>
        <translation>HTTP（未加密）</translation>
    </message>
    <message>
        <source>HTTPS (encrypted)</source>
        <translation>HTTPS（已加密）</translation>
    </message>
    <message>
        <source>Links</source>
        <translation>相关链接</translation>
    </message>
    <message>
        <source>LocalSend v%1</source>
        <translation>LocalSend v%1</translation>
    </message>
    <message>
        <source>Made with &lt;3 for Sailfish OS</source>
        <translation>用 ❤ 为 Sailfish OS 制作</translation>
    </message>
    <message>
        <source>Not affiliated with the LocalSend project. Released under the MIT licence.</source>
        <translation>本项目与 LocalSend 官方项目无关联，基于 MIT 许可证发布。</translation>
    </message>
    <message>
        <source>Protocol</source>
        <translation>协议</translation>
    </message>
    <message>
        <source>Source and issues</source>
        <translation>源码与问题反馈</translation>
    </message>
    <message>
        <source>Stored data</source>
        <translation>已存储数据</translation>
    </message>
    <message>
        <source>The LocalSend project</source>
        <translation>LocalSend 官方项目</translation>
    </message>
    <message>
        <source>This device</source>
        <translation>本机设备</translation>
    </message>
    <message>
        <source>Transfers are encrypted between the two devices with a certificate this phone generated for itself. There is no certificate authority on a local network, so what identifies a device is the fingerprint above: it travels in every announcement, and a device presenting anything else is refused before a single byte is sent.</source>
        <translation>传输使用本机自生成证书做设备间加密。局域网中没有证书颁发机构，设备依靠上方指纹完成身份识别；指纹随每一条设备广播报文发送，如果设备出示不匹配指纹，传输会在任何数据发送前直接拒绝。</translation>
    </message>
    <message>
        <source>Transport</source>
        <translation>传输方式</translation>
    </message>
    <message>
        <source>Version</source>
        <translation>版本</translation>
    </message>
    <message>
        <source>What other devices know this one by. Read it out to somebody to let them confirm it really is you they are sending to.</source>
        <translation>其他设备识别本机的标识。可以念出该指纹，让对方确认接收对象确实是你的设备。</translation>
    </message>
    <message>
        <source>Owner-only files</source>
        <translation>仅本机所有者可见</translation>
    </message>
    <message>
        <source>Encryption is off, so transfers use plain HTTP on port %1 and are readable by anyone who can watch the network. On a home network or your own hotspot that is nobody; on café or office Wi-Fi it may not be.</source>
        <translation>加密已关闭，传输使用 %1 端口的明文 HTTP，任何能监听网络的人都能看到内容。在家庭网络或你自己的热点上没有外人；在咖啡馆或办公 Wi-Fi 上则未必。</translation>
    </message>
    <message>
        <source>Devices find each other with multicast. Plenty of networks block it — guest Wi-Fi almost always does. When that happens, Scan network on the main page finds them the slow way instead.</source>
        <translation>设备通过组播互相发现。很多网络会屏蔽组播——访客 Wi-Fi 几乎总是如此。出现这种情况时，可在主页面用“扫描网络”以较慢的方式找到它们。</translation>
    </message>
</context>
<context>
    <name>AddDevicePage</name>
    <message>
        <source>Add by address</source>
        <translation>通过地址添加设备</translation>
    </message>
    <message>
        <source>Address</source>
        <translation>地址</translation>
    </message>
    <message>
        <source>Forget</source>
        <translation>移除记录</translation>
    </message>
    <message>
        <source>Forgetting</source>
        <translation>正在移除</translation>
    </message>
    <message>
        <source>Found %1</source>
        <translation>已找到 %1</translation>
    </message>
    <message>
        <source>Leave this alone unless the other device was moved off the standard port.</source>
        <translation>除非对方设备修改了非标准端口，否则请勿修改此项。</translation>
    </message>
    <message>
        <source>Look for it</source>
        <translation>开始查找</translation>
    </message>
    <message>
        <source>Nothing answered at that address</source>
        <translation>该地址无设备响应</translation>
    </message>
    <message>
        <source>Port</source>
        <translation>端口</translation>
    </message>
    <message>
        <source>Remembered addresses</source>
        <translation>已记住的地址</translation>
    </message>
    <message>
        <source>For a device on another network, behind a VPN, or on a Wi-Fi that keeps clients apart. Both plain and encrypted transports are tried.</source>
        <translation>适用于位于其他网络、VPN 之后，或客户端彼此隔离的 Wi-Fi 上的设备。会同时尝试明文和加密两种传输方式。</translation>
    </message>
</context>
<context>
    <name>BlockedDevicesPage</name>
    <message>
        <source>Block a device from its entry in the list, or when it asks to send you something.</source>
        <translation>你可以在设备列表内屏蔽设备，也可以在收到对方发送请求时直接屏蔽。</translation>
    </message>
    <message>
        <source>Blocked devices</source>
        <translation>已屏蔽设备</translation>
    </message>
    <message>
        <source>Nothing blocked</source>
        <translation>暂无屏蔽设备</translation>
    </message>
    <message>
        <source>Transfers from these are refused without asking, and they are not shown among nearby devices. Blocking follows the key rather than the name, so renaming does not undo it.</source>
        <translation>来自被屏蔽设备的传输请求会直接拒绝，不再弹窗确认，同时不会出现在附近设备列表。屏蔽绑定设备密钥，仅修改设备名称无法解除屏蔽。</translation>
    </message>
    <message>
        <source>Unblock</source>
        <translation>解除屏蔽</translation>
    </message>
    <message>
        <source>Unblocking</source>
        <translation>正在解除屏蔽</translation>
    </message>
    <message>
        <source>Unnamed device</source>
        <translation>未命名设备</translation>
    </message>
</context>
<context>
    <name>CoverPage</name>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n 个文件</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>%n nearby</source>
        <translation>
            <numerusform>附近共 %n 台设备</numerusform>
        </translation>
    </message>
    <message>
        <source>Incoming</source>
        <translation>接收中</translation>
    </message>
    <message>
        <source>No devices</source>
        <translation>未发现设备</translation>
    </message>
    <message>
        <source>Receiving</source>
        <translation>正在接收</translation>
    </message>
    <message>
        <source>Receiving off</source>
        <translation>接收功能已关闭</translation>
    </message>
    <message>
        <source>Sending</source>
        <translation>正在发送</translation>
    </message>
</context>
<context>
    <name>DeviceDelegate</name>
    <message>
        <source>Block this device</source>
        <translation>屏蔽该设备</translation>
    </message>
    <message>
        <source>Choose files to send</source>
        <translation>选择待发送文件</translation>
    </message>
    <message>
        <source>Device details</source>
        <translation>设备详情</translation>
    </message>
    <message>
        <source>Send staged files</source>
        <translation>发送已选文件</translation>
    </message>
    <message>
        <source>This name has used a different key before</source>
        <translation>该设备名称此前使用过其他密钥</translation>
    </message>
</context>
<context>
    <name>HistoryPage</name>
    <message numerus="yes">
        <source>%n transfer(s)</source>
        <translation>
            <numerusform>%n 条传输记录</numerusform>
        </translation>
    </message>
    <message>
        <source>+%1 more</source>
        <translation>还有 %1 条</translation>
    </message>
    <message>
        <source>Clear history</source>
        <translation>清空历史记录</translation>
    </message>
    <message>
        <source>Clearing history</source>
        <translation>正在清空记录</translation>
    </message>
    <message>
        <source>From %1</source>
        <translation>来自 %1</translation>
    </message>
    <message>
        <source>History</source>
        <translation>传输历史</translation>
    </message>
    <message>
        <source>Nothing yet</source>
        <translation>暂无记录</translation>
    </message>
    <message>
        <source>Open folder</source>
        <translation>打开文件夹</translation>
    </message>
    <message>
        <source>Remove</source>
        <translation>删除本条</translation>
    </message>
    <message>
        <source>Removing</source>
        <translation>正在删除</translation>
    </message>
    <message>
        <source>To %1</source>
        <translation>发送至 %1</translation>
    </message>
    <message>
        <source>Transfers you send and receive will be listed here.</source>
        <translation>所有发送与接收的传输记录都会在此列出。</translation>
    </message>
    <message>
        <source>incomplete</source>
        <translation>未完成</translation>
    </message>
</context>
<context>
    <name>MainPage</name>
    <message>
        <source>A device with this name used a different key before. Nothing stops one device on a network from announcing another one&apos;s name.</source>
        <translation>该设备名曾经使用过别的密钥。局域网中设备可以冒充其他设备名称广播。</translation>
    </message>
    <message>
        <source>About</source>
        <translation>关于</translation>
    </message>
    <message>
        <source>Add by address</source>
        <translation>通过地址添加设备</translation>
    </message>
    <message>
        <source>Add files</source>
        <translation>添加文件</translation>
    </message>
    <message>
        <source>Address</source>
        <translation>地址</translation>
    </message>
    <message>
        <source>Before</source>
        <translation>旧记录</translation>
    </message>
    <message>
        <source>Blocking</source>
        <translation>屏蔽操作</translation>
    </message>
    <message>
        <source>Clear selection</source>
        <translation>清空选中项</translation>
    </message>
    <message>
        <source>Compare the fingerprint against the other device before continuing. Sending confirms the new key and it will not ask again.</source>
        <translation>继续操作前请核对对方设备指纹。一旦执行发送，将确认新密钥，后续不会再次弹窗询问。</translation>
    </message>
    <message>
        <source>Device</source>
        <translation>设备</translation>
    </message>
    <message>
        <source>Device name</source>
        <translation>设备名称</translation>
    </message>
    <message>
        <source>Encrypted</source>
        <translation>已加密</translation>
    </message>
    <message>
        <source>Fingerprint</source>
        <translation>指纹</translation>
    </message>
    <message>
        <source>First time</source>
        <translation>首次发现</translation>
    </message>
    <message>
        <source>History</source>
        <translation>传输历史</translation>
    </message>
    <message>
        <source>LocalSend</source>
        <translation>LocalSend</translation>
    </message>
    <message>
        <source>Look again</source>
        <translation>重新查找</translation>
    </message>
    <message>
        <source>Model</source>
        <translation>设备型号</translation>
    </message>
    <message>
        <source>Nearby devices</source>
        <translation>附近设备</translation>
    </message>
    <message>
        <source>Nobody yet</source>
        <translation>尚未发现其他设备</translation>
    </message>
    <message>
        <source>Not encrypted</source>
        <translation>未加密</translation>
    </message>
    <message>
        <source>Not listening</source>
        <translation>未开启监听</translation>
    </message>
    <message>
        <source>Now</source>
        <translation>当前</translation>
    </message>
    <message>
        <source>Open LocalSend on another device on the same network. It should turn up here within a few seconds. If it does not, pull down: Search every address goes through the whole subnet, and Add by address reaches one that is somewhere else entirely.</source>
        <translation>请在同一网络的其他设备打开 LocalSend，数秒内就会显示在这里。如果没有出现，下拉页面：选择扫描全部地址遍历整个子网，或者通过地址添加跨网段设备。</translation>
    </message>
    <message>
        <source>Port %1 is unavailable</source>
        <translation>端口 %1 不可用</translation>
    </message>
    <message>
        <source>Ready on port %1</source>
        <translation>已就绪，监听端口 %1</translation>
    </message>
    <message>
        <source>Ready · %1 on port %2</source>
        <translation>已就绪 · %1，端口 %2</translation>
    </message>
    <message>
        <source>Receiving is off — others cannot send to you</source>
        <translation>接收已关闭，其他设备无法向你发送文件</translation>
    </message>
    <message>
        <source>Receiving is off. Turn it back on in Settings to be found.</source>
        <translation>接收功能已关闭，请前往设置开启，其他设备才能发现本机。</translation>
    </message>
    <message>
        <source>Scanning the network… %1%</source>
        <translation>正在扫描网络…… %1%</translation>
    </message>
    <message>
        <source>Search every address</source>
        <translation>扫描全部地址</translation>
    </message>
    <message>
        <source>Seen before</source>
        <translation>曾经见过</translation>
    </message>
    <message>
        <source>Select files to send</source>
        <translation>选择待发送文件</translation>
    </message>
    <message>
        <source>Send anyway</source>
        <translation>仍然发送</translation>
    </message>
    <message>
        <source>Send anyway?</source>
        <translation>确定要继续发送吗？</translation>
    </message>
    <message>
        <source>Send files</source>
        <translation>发送文件</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>设置</translation>
    </message>
    <message>
        <source>Shown to other devices</source>
        <translation>对外显示名称</translation>
    </message>
    <message>
        <source>Stop searching</source>
        <translation>停止搜索</translation>
    </message>
    <message>
        <source>Suggest another</source>
        <translation>建议更换名称</translation>
    </message>
    <message>
        <source>The only thing that identifies a device. Compare it against what the other device shows to be certain who you are talking to.</source>
        <translation>这是设备唯一身份凭证，请与对方设备显示的指纹比对，确认通信对象。</translation>
    </message>
    <message>
        <source>This network is blocking discovery. Pull down and choose Scan network.</source>
        <translation>当前网络屏蔽设备发现报文，请下拉页面执行扫描网络。</translation>
    </message>
    <message>
        <source>Transport</source>
        <translation>传输方式</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>类型</translation>
    </message>
    <message>
        <source>Unknown</source>
        <translation>未知</translation>
    </message>
    <message>
        <source>Yes, key matches</source>
        <translation>密钥匹配无误</translation>
    </message>
    <message>
        <source>You have sent to a device called %1 before, but it used a different key. Either it was reinstalled, or something else on this network is using its name.</source>
        <translation>你之前曾经向 %1 发送过文件，但现在该设备使用了不同密钥。可能对方重装应用，也可能网络内其他设备冒用该名称。</translation>
    </message>
    <message>
        <source>Your device name</source>
        <translation>本机设备名称</translation>
    </message>
</context>
<context>
    <name>PinDialog</name>
    <message>
        <source>%1 is asking for a PIN before accepting files.</source>
        <translation>设备 %1 需要校验 PIN 码才接收文件。</translation>
    </message>
    <message>
        <source>PIN</source>
        <translation>PIN 码</translation>
    </message>
    <message>
        <source>PIN required</source>
        <translation>需要 PIN 码</translation>
    </message>
    <message>
        <source>Send</source>
        <translation>发送</translation>
    </message>
    <message>
        <source>That code was not accepted. Try again.</source>
        <translation>PIN 码校验失败，请重新输入。</translation>
    </message>
</context>
<context>
    <name>ReceiveRequestPage</name>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n 个文件</numerusform>
        </translation>
    </message>
    <message>
        <source>Accept</source>
        <translation>接收</translation>
    </message>
    <message>
        <source>Decline</source>
        <translation>拒绝</translation>
    </message>
    <message>
        <source>Decline and block</source>
        <translation>拒绝并屏蔽</translation>
    </message>
    <message>
        <source>Incoming files</source>
        <translation>收到文件请求</translation>
    </message>
    <message>
        <source>Saved to %1</source>
        <translation>已保存至 %1</translation>
    </message>
    <message>
        <source>What they are sending</source>
        <translation>对方将要发送：</translation>
    </message>
    <message>
        <source>from %1</source>
        <translation>来自 %1</translation>
    </message>
</context>
<context>
    <name>ReceiveService</name>
    <message>
        <source>%1 of %2 files were received</source>
        <translation>已接收 %2 个文件中的 %1 个</translation>
    </message>
    <message>
        <source>Cannot write the file</source>
        <translation>无法写入该文件</translation>
    </message>
    <message>
        <source>Cannot write to the destination folder</source>
        <translation>无法写入目标文件夹</translation>
    </message>
    <message>
        <source>No free name for this file</source>
        <translation>没有可用文件名，文件无法保存</translation>
    </message>
    <message>
        <source>Nothing was received</source>
        <translation>未收到任何文件</translation>
    </message>
    <message>
        <source>The file arrived damaged</source>
        <translation>接收到损坏的文件</translation>
    </message>
    <message>
        <source>The sender disconnected</source>
        <translation>发送方已断开连接</translation>
    </message>
    <message>
        <source>Transfer stopped</source>
        <translation>传输已终止</translation>
    </message>
    <message>
        <source>Transfer was cut short</source>
        <translation>传输中途被中断</translation>
    </message>
</context>
<context>
    <name>SelectionPage</name>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n 个文件</numerusform>
        </translation>
    </message>
    <message>
        <source>Add more files</source>
        <translation>继续添加文件</translation>
    </message>
    <message>
        <source>Clear all</source>
        <translation>全部清空</translation>
    </message>
    <message>
        <source>Nothing staged</source>
        <translation>尚未选择任何文件</translation>
    </message>
    <message>
        <source>Pull down to add files</source>
        <translation>下拉页面以添加文件</translation>
    </message>
    <message>
        <source>Ready to send</source>
        <translation>待发送就绪</translation>
    </message>
    <message>
        <source>Remove</source>
        <translation>移除</translation>
    </message>
    <message>
        <source>Removing</source>
        <translation>正在移除</translation>
    </message>
    <message>
        <source>Select files to send</source>
        <translation>选择待发送文件</translation>
    </message>
</context>
<context>
    <name>SelectionTray</name>
    <message numerus="yes">
        <source>%n file(s) ready</source>
        <translation>
            <numerusform>已准备好 %n 个文件</numerusform>
        </translation>
    </message>
    <message>
        <source>pick a device to send</source>
        <translation>请选择目标设备发送</translation>
    </message>
</context>
<context>
    <name>SendService</name>
    <message>
        <source>%1 is busy with another transfer</source>
        <translation>%1 正在处理另一项传输任务</translation>
    </message>
    <message>
        <source>%1 is refusing new requests</source>
        <translation>%1 拒绝新的传输请求</translation>
    </message>
    <message>
        <source>%1 of %2 files were sent</source>
        <translation>已发送 %2 个文件中的 %1 个</translation>
    </message>
    <message>
        <source>%1 refused the transfer (%2)</source>
        <translation>%1 拒绝本次传输（%2）</translation>
    </message>
    <message>
        <source>%1 sent an unreadable reply</source>
        <translation>%1 返回无法解析的应答</translation>
    </message>
    <message>
        <source>%1 stopped the transfer</source>
        <translation>%1 终止了本次传输</translation>
    </message>
    <message>
        <source>Could not open the file</source>
        <translation>无法打开待发送文件</translation>
    </message>
    <message>
        <source>Could not reach %1</source>
        <translation>无法连接到 %1</translation>
    </message>
    <message>
        <source>Nothing was sent</source>
        <translation>没有文件被发送</translation>
    </message>
    <message>
        <source>The transfer was stopped</source>
        <translation>传输已被终止</translation>
    </message>
    <message>
        <source>This device is blocked</source>
        <translation>该设备已被屏蔽</translation>
    </message>
</context>
<context>
    <name>SettingsPage</name>
    <message numerus="yes">
        <source>%n device(s)</source>
        <translation>
            <numerusform>%n 台设备</numerusform>
        </translation>
    </message>
    <message>
        <source>4 to 8 digits</source>
        <translation>4‑8 位数字</translation>
    </message>
    <message>
        <source>53317 is the standard. Change it only if something else is using the port.</source>
        <translation>标准端口为 53317，仅当端口被其他程序占用时再修改。</translation>
    </message>
    <message>
        <source>A folder per sender</source>
        <translation>为每个发送者创建独立文件夹</translation>
    </message>
    <message>
        <source>A notification when a transfer arrives or finishes in the background.</source>
        <translation>后台收到传输请求或传输完成时弹出通知。</translation>
    </message>
    <message>
        <source>Accept without asking</source>
        <translation>自动接收，不再询问</translation>
    </message>
    <message>
        <source>Allow incoming files</source>
        <translation>允许接收传入文件</translation>
    </message>
    <message>
        <source>Blocked devices</source>
        <translation>已屏蔽设备</translation>
    </message>
    <message>
        <source>Change the PIN</source>
        <translation>修改 PIN 码</translation>
    </message>
    <message>
        <source>Device name</source>
        <translation>设备名称</translation>
    </message>
    <message>
        <source>Encrypt transfers</source>
        <translation>启用传输加密</translation>
    </message>
    <message>
        <source>Files are encrypted between the two devices. Turning this off makes every transfer readable by anyone on the same network.</source>
        <translation>设备之间文件传输加密。关闭加密，同一局域网下其他人可以窃听全部传输内容。</translation>
    </message>
    <message>
        <source>Files are saved as soon as they arrive. Convenient at home, unwise on a network you share.</source>
        <translation>文件到达直接保存。家庭内网很方便，公共共享网络不建议开启。</translation>
    </message>
    <message>
        <source>Interface language</source>
        <translation>界面语言</translation>
    </message>
    <message>
        <source>Keep a history</source>
        <translation>保存传输历史记录</translation>
    </message>
    <message>
        <source>Keep going with the screen off</source>
        <translation>熄屏后继续运行传输</translation>
    </message>
    <message>
        <source>Language</source>
        <translation>语言</translation>
    </message>
    <message>
        <source>Listening port</source>
        <translation>监听端口</translation>
    </message>
    <message>
        <source>None</source>
        <translation>无</translation>
    </message>
    <message>
        <source>Not set</source>
        <translation>未设置</translation>
    </message>
    <message>
        <source>Notify me</source>
        <translation>发送通知提醒我</translation>
    </message>
    <message>
        <source>Only a salted hash of the code is stored, so it can be changed but never shown again.</source>
        <translation>只会存储加盐哈希，PIN 码可以修改，但不会明文显示。</translation>
    </message>
    <message>
        <source>Other LocalSend devices look on 53317 by default. A different port still works, but only if the other side is told about it.</source>
        <translation>其他 LocalSend 默认使用 53317 端口。修改端口后，对方也需要使用对应端口才能连通。</translation>
    </message>
    <message>
        <source>PIN</source>
        <translation>PIN 码</translation>
    </message>
    <message>
        <source>Port</source>
        <translation>端口</translation>
    </message>
    <message>
        <source>Received files go into a subfolder named after the device that sent them.</source>
        <translation>接收到的文件将存放在以发送设备命名的子文件夹内。</translation>
    </message>
    <message>
        <source>Receiving</source>
        <translation>接收设置</translation>
    </message>
    <message>
        <source>Records what was sent and received, and where it was saved.</source>
        <translation>记录发送、接收任务以及文件保存路径。</translation>
    </message>
    <message>
        <source>Refused before you are asked, and hidden from the device list.</source>
        <translation>请求直接拒绝，不再弹窗，设备不会出现在附近列表。</translation>
    </message>
    <message>
        <source>Remove the PIN</source>
        <translation>清除 PIN 码</translation>
    </message>
    <message>
        <source>Require a PIN</source>
        <translation>启用 PIN 码校验</translation>
    </message>
    <message>
        <source>Save to</source>
        <translation>保存路径</translation>
    </message>
    <message>
        <source>Saving</source>
        <translation>保存中</translation>
    </message>
    <message>
        <source>Security</source>
        <translation>安全选项</translation>
    </message>
    <message>
        <source>Senders must enter this code before you are even asked.</source>
        <translation>发送方必须输入该 PIN 码，才会弹出接收确认。</translation>
    </message>
    <message>
        <source>Set</source>
        <translation>设置</translation>
    </message>
    <message>
        <source>Set a PIN</source>
        <translation>设置 PIN 码</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>设置</translation>
    </message>
    <message>
        <source>Stored as a salted hash, so it cannot be shown again — only replaced.</source>
        <translation>PIN 以加盐哈希保存，无法查看明文，仅支持重新设置。</translation>
    </message>
    <message>
        <source>Suggest another name</source>
        <translation>建议更换名称</translation>
    </message>
    <message>
        <source>The app reloads when this changes.</source>
        <translation>修改此项后应用会重新加载。</translation>
    </message>
    <message>
        <source>This device</source>
        <translation>本机设备</translation>
    </message>
    <message>
        <source>This device is identified by the fingerprint of its certificate, so changing this setting makes it look like a new device to everyone else.</source>
        <translation>设备身份依靠证书指纹识别，修改此项设置后，其他设备会把本机识别为全新设备。</translation>
    </message>
    <message>
        <source>Unavailable on this device: %1</source>
        <translation>本机不支持该功能：%1</translation>
    </message>
    <message>
        <source>What other devices call you.</source>
        <translation>其他设备看到的本机名称。</translation>
    </message>
    <message>
        <source>When off, this device stops announcing itself and refuses transfers.</source>
        <translation>关闭后本机停止广播设备信息，拒绝一切传入传输。</translation>
    </message>
    <message>
        <source>Where to save incoming files</source>
        <translation>接收文件的保存目录</translation>
    </message>
    <message>
        <source>While transferring</source>
        <translation>传输过程</translation>
    </message>
    <message>
        <source>Stops the device suspending mid-transfer. Uses more battery.</source>
        <translation>防止设备在传输过程中休眠，会更耗电。</translation>
    </message>
</context>
<context>
    <name>TransferFileDelegate</name>
    <message>
        <source>failed</source>
        <translation>失败</translation>
    </message>
    <message>
        <source>skipped</source>
        <translation>已跳过</translation>
    </message>
</context>
<context>
    <name>TransferPage</name>
    <message>
        <source>%1 · %2 left</source>
        <translation>%1 · 剩余 %2</translation>
    </message>
    <message>
        <source>Done</source>
        <translation>完成</translation>
    </message>
    <message>
        <source>Files</source>
        <translation>文件列表</translation>
    </message>
    <message>
        <source>Open</source>
        <translation>打开</translation>
    </message>
    <message>
        <source>Receiving</source>
        <translation>正在接收</translation>
    </message>
    <message numerus="yes">
        <source>Saved %n file(s)</source>
        <translation>
            <numerusform>已保存 %n 个文件</numerusform>
        </translation>
    </message>
    <message>
        <source>Sending</source>
        <translation>正在发送</translation>
    </message>
    <message numerus="yes">
        <source>Sent %n file(s)</source>
        <translation>
            <numerusform>已发送 %n 个文件</numerusform>
        </translation>
    </message>
    <message>
        <source>Starting…</source>
        <translation>正在启动……</translation>
    </message>
    <message>
        <source>Stop</source>
        <translation>停止</translation>
    </message>
    <message>
        <source>Transfer failed</source>
        <translation>传输失败</translation>
    </message>
    <message>
        <source>Transfer stopped</source>
        <translation>传输已停止</translation>
    </message>
    <message>
        <source>Waiting for %1 to accept</source>
        <translation>等待 %1 确认接收</translation>
    </message>
    <message>
        <source>in %1</source>
        <translation>保存至 %1</translation>
    </message>
</context>
<context>
    <name>harbour-localsend</name>
    <message>
        <source>%1 wants to send you files</source>
        <translation>%1 想要向你发送文件</translation>
    </message>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n 个文件</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>%n file(s) received</source>
        <translation>
            <numerusform>已接收 %n 个文件</numerusform>
        </translation>
    </message>
    <message>
        <source>Transfer incomplete</source>
        <translation>传输未完成</translation>
    </message>
    <message numerus="yes">
        <source>%n file(s) sent</source>
        <translation>
            <numerusform>已发送 %n 个文件</numerusform>
        </translation>
    </message>
</context>
</TS>
