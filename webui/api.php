<?php
/**
 * Magic Cam WebUI API
 * Handles communication between WebUI and Android system
 */

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

$request_uri = $_SERVER['REQUEST_URI'];
$method = $_SERVER['REQUEST_METHOD'];

// Route handling
switch (true) {
    case preg_match('/\/api\/apps$/', $request_uri) && $method === 'GET':
        getInstalledApps();
        break;
        
    case preg_match('/\/api\/config$/', $request_uri) && $method === 'GET':
        loadConfig();
        break;
        
    case preg_match('/\/api\/config$/', $request_uri) && $method === 'POST':
        saveConfig();
        break;
        
    case preg_match('/\/api\/upload$/', $request_uri) && $method === 'POST':
        uploadVideo();
        break;
        
    case preg_match('/\/api\/status$/', $request_uri) && $method === 'GET':
        getModuleStatus();
        break;
        
    case preg_match('/\/api\/toggle$/', $request_uri) && $method === 'POST':
        toggleModule();
        break;
        
    default:
        http_response_code(404);
        echo json_encode(['error' => 'Endpoint not found']);
        break;
}

function getInstalledApps() {
    try {
        // Use Android package manager to get installed apps
        $cmd = 'pm list packages -f 2>/dev/null';
        $packages = shell_exec($cmd);
        
        if (!$packages) {
            throw new Exception('Failed to get package list');
        }
        
        $apps = [];
        $lines = explode("\n", trim($packages));
        
        foreach ($lines as $line) {
            if (preg_match('/package:(.+)=(.+)/', $line, $matches)) {
                $apkPath = $matches[1];
                $packageName = $matches[2];
                
                // Get app label
                $labelCmd = "dumpsys package $packageName | grep -A1 'applicationLabel' | tail -1 | cut -d'=' -f2";
                $label = trim(shell_exec($labelCmd));
                
                // Check if system app
                $isSystemApp = strpos($apkPath, '/system/') === 0;
                
                // Get permissions
                $permCmd = "dumpsys package $packageName | grep 'android.permission' | grep -o 'android.permission[^:]*'";
                $permissions = array_filter(explode("\n", trim(shell_exec($permCmd))));
                
                $apps[] = [
                    'packageName' => $packageName,
                    'label' => $label ?: $packageName,
                    'apkPath' => $apkPath,
                    'isSystemApp' => $isSystemApp,
                    'permissions' => $permissions
                ];
            }
        }
        
        // Sort by label
        usort($apps, function($a, $b) {
            return strcasecmp($a['label'], $b['label']);
        });
        
        echo json_encode($apps);
        
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => 'Failed to load apps: ' . $e->getMessage()]);
    }
}

function loadConfig() {
    $configFile = '/data/adb/modules/com_twj_mc/webui_config.json';
    
    if (file_exists($configFile)) {
        $config = json_decode(file_get_contents($configFile), true);
        echo json_encode($config ?: []);
    } else {
        // Return default config
        echo json_encode([
            'moduleEnabled' => true,
            'virtualMode' => true,
            'videoPath' => '',
            'videoQuality' => '1080p',
            'loopVideo' => true,
            'detectionBypass' => true,
            'debugLogging' => false,
            'hookMethod' => 'camera2ndk',
            'selectedApps' => []
        ]);
    }
}

function saveConfig() {
    try {
        $input = json_decode(file_get_contents('php://input'), true);
        
        if (!$input) {
            throw new Exception('Invalid JSON input');
        }
        
        $configFile = '/data/adb/modules/com_twj_mc/webui_config.json';
        $configDir = dirname($configFile);
        
        if (!is_dir($configDir)) {
            mkdir($configDir, 0755, true);
        }
        
        if (file_put_contents($configFile, json_encode($input, JSON_PRETTY_PRINT)) === false) {
            throw new Exception('Failed to write config file');
        }
        
        // Apply configuration to module
        applyConfiguration($input);
        
        echo json_encode(['success' => true]);
        
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => 'Failed to save config: ' . $e->getMessage()]);
    }
}

function uploadVideo() {
    try {
        if (!isset($_FILES['video'])) {
            throw new Exception('No video file uploaded');
        }
        
        $file = $_FILES['video'];
        
        if ($file['error'] !== UPLOAD_ERR_OK) {
            throw new Exception('Upload error: ' . $file['error']);
        }
        
        // Validate file type
        $allowedTypes = ['video/mp4', 'video/avi', 'video/mov', 'video/mkv'];
        if (!in_array($file['type'], $allowedTypes)) {
            throw new Exception('Invalid file type. Only MP4, AVI, MOV, and MKV are supported.');
        }
        
        // Create MagicCam directory
        $uploadDir = '/sdcard/MagicCam';
        if (!is_dir($uploadDir)) {
            mkdir($uploadDir, 0755, true);
        }
        
        $fileName = basename($file['name']);
        $uploadPath = $uploadDir . '/' . $fileName;
        
        if (!move_uploaded_file($file['tmp_name'], $uploadPath)) {
            throw new Exception('Failed to move uploaded file');
        }
        
        // Set proper permissions
        chmod($uploadPath, 0644);
        
        echo json_encode([
            'success' => true,
            'path' => $uploadPath,
            'filename' => $fileName
        ]);
        
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => 'Upload failed: ' . $e->getMessage()]);
    }
}

function getModuleStatus() {
    try {
        // Check if module is loaded
        $moduleLoaded = file_exists('/data/adb/modules/com_twj_mc/module.prop');
        
        // Check if Zygisk is enabled
        $zygiskEnabled = file_exists('/data/adb/magisk/zygisk') || 
                        shell_exec('magisk --sqlite "SELECT value FROM settings WHERE key=\'zygisk\'"') === '1';
        
        // Get current configuration
        $config = json_decode(file_get_contents('/data/adb/modules/com_twj_mc/webui_config.json') ?: '{}', true);
        
        echo json_encode([
            'enabled' => $moduleLoaded && $zygiskEnabled,
            'virtualMode' => $config['virtualMode'] ?? true,
            'moduleLoaded' => $moduleLoaded,
            'zygiskEnabled' => $zygiskEnabled,
            'hookedApps' => count($config['selectedApps'] ?? [])
        ]);
        
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => 'Failed to get status: ' . $e->getMessage()]);
    }
}

function toggleModule() {
    try {
        $input = json_decode(file_get_contents('php://input'), true);
        $enabled = $input['enabled'] ?? false;
        
        $moduleDir = '/data/adb/modules/com_twj_mc';
        $disableFile = $moduleDir . '/disable';
        
        if ($enabled) {
            // Enable module by removing disable file
            if (file_exists($disableFile)) {
                unlink($disableFile);
            }
        } else {
            // Disable module by creating disable file
            file_put_contents($disableFile, '');
        }
        
        echo json_encode(['success' => true, 'enabled' => $enabled]);
        
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => 'Failed to toggle module: ' . $e->getMessage()]);
    }
}

function applyConfiguration($config) {
    // Write configuration to module files
    $moduleDir = '/data/adb/modules/com_twj_mc';
    
    // Create target apps list
    $targetAppsFile = $moduleDir . '/target_apps.txt';
    if (!empty($config['selectedApps'])) {
        file_put_contents($targetAppsFile, implode("\n", $config['selectedApps']));
    }
    
    // Create settings file for native module
    $settingsFile = $moduleDir . '/settings.conf';
    $settings = [
        'virtual_mode=' . ($config['virtualMode'] ? '1' : '0'),
        'video_path=' . ($config['videoPath'] ?: ''),
        'video_quality=' . ($config['videoQuality'] ?: '1080p'),
        'loop_video=' . ($config['loopVideo'] ? '1' : '0'),
        'detection_bypass=' . ($config['detectionBypass'] ? '1' : '0'),
        'debug_logging=' . ($config['debugLogging'] ? '1' : '0'),
        'hook_method=' . ($config['hookMethod'] ?: 'camera2ndk')
    ];
    
    file_put_contents($settingsFile, implode("\n", $settings));
    
    // Set proper permissions
    chmod($targetAppsFile, 0644);
    chmod($settingsFile, 0644);
}
?>