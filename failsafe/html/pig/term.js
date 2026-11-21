(function () {
	'use strict';
	var form = document.getElementById('terminal');
	var command = document.getElementById('command');
	var output = document.getElementById('output');
	var execute = document.getElementById('execute');
	var abort = document.getElementById('abort');
	var status = document.getElementById('terminalStatus');
	var prefix = '';
	var headers = {'X-BE7000-Request': '1', 'Content-Type': 'text/plain'};
	var polling = false;

	function unlock() {
		polling = false;
		execute.disabled = false;
		execute.textContent = '执行';
		command.required = true;
		abort.disabled = true;
		command.focus();
	}

	function update() {
		fetch('/terminal/output', {cache: 'no-store'}).then(function (response) {
			if (!response.ok) throw new Error();
			return response.text();
		}).then(function (text) {
			output.textContent = prefix + text;
			output.scrollTop = output.scrollHeight;
			if (/\n\[exit -?\d+\]\n$/.test(text)) {
				status.textContent = '命令已结束。';
				unlock();
			} else if (polling) {
				setTimeout(update, 700);
			}
		}).catch(function () {
			status.textContent = '连接已中断。';
			unlock();
		});
	}

	if (location.protocol !== 'https:') {
		status.textContent = '请使用配对后的 HTTPS 恢复页面。';
		execute.disabled = true;
		return;
	}

	form.addEventListener('submit', function (event) {
		event.preventDefault();
		var text = command.value;
		if (polling) {
			fetch('/terminal/input', {method: 'POST', headers: headers, body: text + '\r'}).then(function (response) {
				if (!response.ok) throw new Error();
				command.value = '';
			}).catch(function () {
				status.textContent = '暂时无法发送输入。';
			});
			return;
		}
		if (!text.trim()) return;
		execute.disabled = true;
		abort.disabled = false;
		polling = true;
		fetch('/terminal/command', {method: 'POST', headers: headers, body: text}).then(function (response) {
			if (!response.ok) throw new Error();
			prefix = '=> ' + text + '\n';
			output.textContent = prefix;
			command.value = '';
			command.required = false;
			execute.textContent = '发送输入';
			execute.disabled = false;
			status.textContent = '正在执行…';
			update();
		}).catch(function () {
			status.textContent = '设备忙，或命令未能送达。';
			unlock();
		});
	});

	abort.addEventListener('click', function () {
		fetch('/terminal/abort', {method: 'POST', headers: headers, body: ''}).then(function (response) {
			if (!response.ok) throw new Error();
			status.textContent = '已请求中断。';
		}).catch(function () {
			status.textContent = '中断请求未能送达。';
		});
	});
}());
