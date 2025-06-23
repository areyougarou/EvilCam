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
        uploadMedia();
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
    $configFile = '/data/adb/modules/twj_mc/webui_config.json';
    
    if (file_exists($configFile)) {
        $config = json_decode(file_get_contents($configFile), true);
        echo json_encode($config ?: []);
    } else {
        // Return default config
        echo json_encode([
            'moduleEnabled' => true,
            'virtualMode' => true,
            'mediaType' => 'video',
            'videoPath' => '',
            'photoPath' => '',
            'videoQuality' => '1080p',
            'photoResolution' => '1080p',
            'aspectRatio' => '16:9',
            'photoEffects' => 'none',
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
        
        $configFile = '/data/adb/modules/twj_mc/webui_config.json';
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

function uploadMedia() {
    try {
        $uploadedFile = null;
        $fileType = null;
        
        // Check for video upload
        if (isset($_FILES['video'])) {
            $uploadedFile = $_FILES['video'];
            $fileType = 'video';
            $allowedTypes = ['video/mp4', 'video/avi', 'video/mov', 'video/mkv', 'video/webm'];
        }
        // Check for photo upload
        elseif (isset($_FILES['photo'])) {
            $uploadedFile = $_FILES['photo'];
            $fileType = 'photo';
            $allowedTypes = ['image/jpeg', 'image/jpg', 'image/png', 'image/gif', 'image/bmp', 'image/webp'];
        }
        else {
            throw new Exception('No media file uploaded');
        }
        
        if ($uploadedFile['error'] !== UPLOAD_ERR_OK) {
            throw new Exception('Upload error: ' . $uploadedFile['error']);
        }
        
        // Validate file type
        if (!in_array($uploadedFile['type'], $allowedTypes)) {
            $supportedFormats = $fileType === 'video' ? 'MP4, AVI, MOV, MKV, WebM' : 'JPEG, PNG, GIF, BMP, WebP';
            throw new Exception("Invalid file type. Only $supportedFormats are supported.");
        }
        
        // NO SIZE LIMITS - Accept any file size
        
        // Create MagicCam directory
        $uploadDir = '/sdcard/MagicCam';
        if (!is_dir($uploadDir)) {
            mkdir($uploadDir, 0755, true);
        }
        
        // Generate unique filename to avoid conflicts
        $fileName = time() . '_' . basename($uploadedFile['name']);
        $uploadPath = $uploadDir . '/' . $fileName;
        
        if (!move_uploaded_file($uploadedFile['tmp_name'], $uploadPath)) {
            throw new Exception('Failed to move uploaded file');
        }
        
        // Set proper permissions
        chmod($uploadPath, 0644);
        
        // For photos, create thumbnail
        if ($fileType === 'photo') {
            createThumbnail($uploadPath, $uploadDir . '/thumb_' . $fileName);
        }
        
        echo json_encode([
            'success' => true,
            'path' => $uploadPath,
            'filename' => $fileName,
            'type' => $fileType,
            'size' => $uploadedFile['size']
        ]);
        
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => 'Upload failed: ' . $e->getMessage()]);
    }
}

function createThumbnail($sourcePath, $thumbPath) {
    try {
        $imageInfo = getimagesize($sourcePath);
        if (!$imageInfo) return false;
        
        $sourceWidth = $imageInfo[0];
        $sourceHeight = $imageInfo[1];
        $mimeType = $imageInfo['mime'];
        
        // Create source image resource
        switch ($mimeType) {
            case 'image/jpeg':
                $sourceImage = imagecreatefromjpeg($sourcePath);
                break;
            case 'image/png':
                $sourceImage = imagecreatefrompng($sourcePath);
                break;
            case 'image/gif':
                $sourceImage = imagecreatefromgif($sourcePath);
                break;
            default:
                return false;
        }
        
        if (!$sourceImage) return false;
        
        // Calculate thumbnail dimensions (max 200x200)
        $maxSize = 200;
        if ($sourceWidth > $sourceHeight) {
            $thumbWidth = $maxSize;
            $thumbHeight = intval($sourceHeight * $maxSize / $sourceWidth);
        } else {
            $thumbHeight = $maxSize;
            $thumbWidth = intval($sourceWidth * $maxSize / $sourceHeight);
        }
        
        // Create thumbnail
        $thumbImage = imagecreatetruecolor($thumbWidth, $thumbHeight);
        imagecopyresampled($thumbImage, $sourceImage, 0, 0, 0, 0, $thumbWidth, $thumbHeight, $sourceWidth, $sourceHeight);
        
        // Save thumbnail
        imagejpeg($thumbImage, $thumbPath, 80);
        
        // Clean up
        imagedestroy($sourceImage);
        imagedestroy($thumbImage);
        
        return true;
    } catch (Exception $e) {
        return false;
    }
}

function getModuleStatus() {
    try {
        // Check if module is loaded
        $moduleLoaded = file_exists('/data/adb/modules/twj_mc/module.prop');
        
        // Check if Zygisk is enabled
        $zygiskEnabled = file_exists('/data/adb/magisk/zygisk') || 
                        shell_exec('magisk --sqlite "SELECT value FROM settings WHERE key=\'zygisk\'"') === '1';
        
        // Get current configuration
        $configFile = '/data/adb/modules/twj_mc/webui_config.json';
        $config = [];
        if (file_exists($configFile)) {
            $config = json_decode(file_get_contents($configFile), true) ?: [];
        }
        
        echo json_encode([
            'enabled' => $moduleLoaded && $zygiskEnabled,
            'virtualMode' => $config['virtualMode'] ?? true,
            'moduleLoaded' => $moduleLoaded,
            'zygiskEnabled' => $zygiskEnabled,
            'hookedApps' => count($config['selectedApps'] ?? []),
            'mediaType' => $config['mediaType'] ?? 'video'
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
        
        $moduleDir = '/data/adb/modules/twj_mc';
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
    $moduleDir = '/data/adb/modules/twj_mc';
    
    // Create target apps list
    $targetAppsFile = $moduleDir . '/target_apps.txt';
    if (!empty($config['selectedApps'])) {
        file_put_contents($targetAppsFile, implode("\n", $config['selectedApps']));
    }
    
    // Create settings file for native module
    $settingsFile = $moduleDir . '/settings.conf';
    $settings = [
        'virtual_mode=' . ($config['virtualMode'] ? '1' : '0'),
        'media_type=' . ($config['mediaType'] ?: 'video'),
        'video_path=' . ($config['videoPath'] ?: ''),
        'photo_path=' . ($config['photoPath'] ?: ''),
        'video_quality=' . ($config['videoQuality'] ?: '1080p'),
        'photo_resolution=' . ($config['photoResolution'] ?: '1080p'),
        'aspect_ratio=' . ($config['aspectRatio'] ?: '16:9'),
        'photo_effects=' . ($config['photoEffects'] ?: 'none'),
        'loop_video=' . ($config['loopVideo'] ? '1' : '0'),
        'detection_bypass=' . ($config['detectionBypass'] ? '1' : '0'),
        'debug_logging=' . ($config['debugLogging'] ? '1' : '0'),
        'hook_method=' . ($config['hookMethod'] ?: 'camera2ndk')
    ];
    
    file_put_contents($settingsFile, implode("\n", $settings));
    
    // Set proper permissions
    if (file_exists($targetAppsFile)) {
        chmod($targetAppsFile, 0644);
    }
    chmod($settingsFile, 0644);
}
?>