clc; clear; close all;
filename   = 'f2449M_fs61M_20251225_204008.cs16'; 
fs         = 61.44e6;
fft_len   = 4096;
overlap   = round(0.25 * fft_len);
win       = hamming(fft_len);
chunk_len = fft_len * 50;
max_cols  = 1000; 
dyn_range = 90; 
time_per_column = (fft_len - overlap) / fs;
total_display_time = max_cols * time_per_column;
color_map = 'jet';

fid = fopen(filename, 'rb');
if fid == -1
    error('无法打开文件: %s', filename);
end
disp(['正在分析文件: ', filename]);
disp(['采样率: ', num2str(fs/1e6), ' MHz']);
disp(['FFT长度: ', num2str(fft_len)]);
disp('只显示实频图（频谱图/瀑布图）...');
disp(['时间参数: 每列=', sprintf('%.6f', time_per_column), '秒, 总显示=', sprintf('%.3f', total_display_time), '秒']);

fig = figure('Color', 'w', 'Position', [100 100 1400 700], ...
       'Name', 'Inspectrum-like Real Frequency Display', 'NumberTitle', 'off');

h_spec = imagesc(zeros(fft_len, max_cols));
axis xy;  

colormap(color_map);

colorbar('eastoutside');
caxis([-dyn_range 0]);
ylabel('Frequency (MHz)');
xlabel('Time (seconds)');
title(sprintf('High-Resolution Real Frequency Display (Spectrogram/Waterfall)\\nDisplaying %.2f seconds of history', total_display_time));


freq_axis = linspace(-fs/2, fs/2, fft_len) / 1e6;
set(gca, 'YTick', linspace(1, fft_len, 9), ...
         'YTickLabel', round(linspace(freq_axis(1), freq_axis(end), 9), 2));


time_axis = linspace(0, total_display_time, max_cols);
set(gca, 'XTick', linspace(1, max_cols, 6), ...
         'XTickLabel', round(linspace(time_axis(1), time_axis(end), 6), 2));

grid on;
box on;


button_panel = uipanel('Parent', fig, 'Position', [0.85 0.02 0.14 0.08], ...
                       'Title', '控制面板', 'FontSize', 10);

global status_text;
status_text = uicontrol('Parent', button_panel, 'Style', 'text', ...
                        'String', '状态: 运行中', 'Position', [10 45 200 20], ...
                        'FontSize', 10, 'HorizontalAlignment', 'left', ...
                        'BackgroundColor', [0.9 0.9 0.9]);

stop_btn = uicontrol('Parent', button_panel, 'Style', 'pushbutton', ...
                     'String', 'Stop', 'Position', [10 10 60 30], ...
                     'FontSize', 10, 'Callback', @stopCallback);

start_btn = uicontrol('Parent', button_panel, 'Style', 'pushbutton', ...
                      'String', 'Start', 'Position', [80 10 60 30], ...
                      'FontSize', 10, 'Callback', @startCallback);

exit_btn = uicontrol('Parent', button_panel, 'Style', 'pushbutton', ...
                     'String', 'Exit', 'Position', [150 10 60 30], ...
                     'FontSize', 10, 'Callback', @exitCallback);

global is_running;
global is_exit;
is_running = true;
is_exit = false;

drawnow;

spec_buf = -120 * ones(fft_len, max_cols);

frame_count = 0;
disp('开始处理数据... (按ESC键可中断，使用控制按钮停止/继续/退出)');

while ~is_exit
    if ~is_running
        pause(0.1);
        continue;
    end
    raw = fread(fid, 2 * chunk_len, 'int16');
    
    if numel(raw) < 2 * fft_len
        disp('文件读取完成或数据不足');
        break;
    end
    iq = complex(double(raw(1:2:end)), double(raw(2:2:end))) / 32768;

    [S, ~, ~] = spectrogram(iq, win, overlap, fft_len, fs, 'centered');
    
    S_dB = 20 * log10(abs(S) + 1e-12);
    ncol = size(S_dB, 2); 
    
    if ncol >= max_cols
        spec_buf = S_dB(:, end-max_cols+1:end);
    else
        spec_buf(:, 1:end-ncol) = spec_buf(:, ncol+1:end);
        spec_buf(:, end-ncol+1:end) = S_dB;
    end

    set(h_spec, 'CData', spec_buf);
    
    frame_count = frame_count + 1;

    samples_processed = frame_count * chunk_len;
    current_file_time = samples_processed / fs; 
    
    if mod(frame_count, 20) == 0
        title(sprintf('Real Frequency Display | Frame: %d | File Time: %.3f s', ...
              frame_count, current_file_time));
        time_axis = linspace(current_file_time - total_display_time, current_file_time, max_cols);
        set(gca, 'XTick', linspace(1, max_cols, 6), ...
                 'XTickLabel', round(linspace(time_axis(1), time_axis(end), 6), 3));
    end
    
    drawnow limitrate;

    if ~isempty(get(gcf, 'CurrentCharacter'))
        key = get(gcf, 'CurrentCharacter');
        if key == 27
            disp('用户中断: 按ESC键退出');
            is_exit = true;
            break;
        end
        set(gcf, 'CurrentCharacter', char(0));
    end
end

fclose(fid);
samples_processed = frame_count * chunk_len;
current_file_time = samples_processed / fs;
title(sprintf('Real Frequency Display | Frame: %d | File Time: %.3f s', ...
      frame_count, current_file_time));
time_axis = linspace(current_file_time - total_display_time, current_file_time, max_cols);
set(gca, 'XTick', linspace(1, max_cols, 6), ...
         'XTickLabel', round(linspace(time_axis(1), time_axis(end), 6), 3));
drawnow;

disp(['处理完成。总帧数: ', num2str(frame_count)]);
disp(['1. 文件: ', filename]);
disp(['2. 总处理帧数: ', num2str(frame_count)]);
disp(['3. FFT长度: ', num2str(fft_len)]);
disp(['4. 频率范围: ', num2str(freq_axis(1)), ' 到 ', num2str(freq_axis(end)), ' MHz']);
disp(['5. 动态范围: ', num2str(dyn_range), ' dB']);
disp(['6. 文件时间位置: ', sprintf('%.3f', frame_count * chunk_len / fs), ' 秒']);
disp(['7. 时间分辨率: ', sprintf('%.6f', time_per_column), ' 秒/列']);
disp(['8. 显示历史长度: ', sprintf('%.3f', total_display_time), ' 秒']);
function stopCallback(~, ~)
    global is_running;
    global status_text;
    is_running = false;
    set(status_text, 'String', '状态: 已停止');
    disp('处理已停止');
end

function startCallback(~, ~)
    global is_running;
    global status_text;
    is_running = true;
    set(status_text, 'String', '状态: 运行中');
    disp('处理已继续');
end

function exitCallback(~, ~)
    global is_exit;
    global is_running;
    global status_text;
    is_exit = true;
    is_running = false;
    set(status_text, 'String', '状态: 退出中');
    disp('正在退出...');
end
