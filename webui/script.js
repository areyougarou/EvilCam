// WebUI X API Integration
const WEBUI_API = {
    // KernelSU/Magisk module API endpoints
    getInstalledApps: () => fetch('/api/apps').then(r => r.json()),
    saveConfig: (config) => fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    }),
    loadConfig: () => fetch('/api/config').then(r => r.json()),
    uploadVideo: (file) => {
        const formData = new FormData();
        formData.append('video', file);
        return fetch('/api/upload', {
            method: 'POST',
            body: formData
        });
    },
    getModuleStatus: () => fetch('/api/status').then(r => r.json()),
    toggleModule: (enabled) => fetch('/api/toggle', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled })
    })
};

// Application state
let appState = {
    apps: [],
    selectedApps: new Set(),
    currentCategory: 'all',
    searchQuery: '',
    config: {
        moduleEnabled: true,
        virtualMode: true,
        videoPath: '',
        videoQuality: '1080p',
        loopVideo: true,
        detectionBypass: true,
        debugLogging: false,
        hookMethod: 'camera2ndk'
    }
};

// Initialize the application
document.addEventListener('DOMContentLoaded', async () => {
    await initializeApp();
    setupEventListeners();
    await loadSystemApps();
    await loadConfiguration();
    updateUI();
});

async function initializeApp() {
    console.log('Initializing Magic Cam WebUI...');
    
    // Show loading state
    showMessage('Loading system applications...', 'info');
    
    try {
        // Check module status
        const status = await WEBUI_API.getModuleStatus();
        updateModuleStatus(status);
    } catch (error) {
        console.error('Failed to get module status:', error);
        showMessage('Failed to connect to module API', 'error');
    }
}

async function loadSystemApps() {
    try {
        console.log('Loading apps from Android system...');
        
        // Load apps from system via WebUI API - NO FALLBACK
        const systemApps = await WEBUI_API.getInstalledApps();
        
        if (!systemApps || !Array.isArray(systemApps) || systemApps.length === 0) {
            throw new Error('No system apps loaded - system integration failed');
        }
        
        // Filter to only show system apps as requested
        const filteredSystemApps = systemApps.filter(app => app.isSystemApp === true);
        
        if (filteredSystemApps.length === 0) {
            throw new Error('No system apps found - system integration failed');
        }
        
        // Process and categorize apps dynamically
        appState.apps = filteredSystemApps.map(app => ({
            name: app.label || app.packageName,
            package: app.packageName,
            category: categorizeApp(app.packageName),
            icon: getAppIcon(app.packageName),
            isSystem: true, // All apps are system apps now
            hasCamera: app.permissions?.includes('android.permission.CAMERA') || false
        }));
        
        // Sort apps by name
        appState.apps.sort((a, b) => a.name.localeCompare(b.name));
        
        console.log(`Loaded ${appState.apps.length} system applications from Android`);
        renderAppList();
        showMessage(`Successfully loaded ${appState.apps.length} system apps`, 'success');
        
    } catch (error) {
        console.error('CRITICAL: System app loading failed:', error);
        
        // NO FALLBACK - App becomes non-functional as requested
        showMessage('CRITICAL: System app loading failed - WebUI non-functional', 'error');
        document.getElementById('appList').innerHTML = `
            <div class="error-state">
                <h3>❌ System Integration Failed</h3>
                <p>Unable to load applications from Android system.</p>
                <p>WebUI cannot function without system app access.</p>
                <p>Error: ${error.message}</p>
            </div>
        `;
        
        // Disable all functionality
        document.querySelectorAll('button, input, select').forEach(element => {
            element.disabled = true;
        });
        
        // Clear apps array
        appState.apps = [];
    }
}

function categorizeApp(packageName) {
    const categories = {
        camera: [
            'camera', 'cam', 'photo', 'picture', 'snap'
        ],
        media: [
            'media', 'gallery', 'video', 'audio', 'music'
        ],
        system: [
            'android', 'system', 'framework', 'provider'
        ],
        communication: [
            'phone', 'contacts', 'messaging', 'dialer'
        ]
    };
    
    const pkg = packageName.toLowerCase();
    
    for (const [category, keywords] of Object.entries(categories)) {
        if (keywords.some(keyword => pkg.includes(keyword))) {
            return category;
        }
    }
    
    return 'system'; // Default to system for system apps
}

function getAppIcon(packageName) {
    const iconMap = {
        // System Camera apps
        'com.android.camera': '📷',
        'com.android.camera2': '📸',
        'com.google.android.GoogleCamera': '📷',
        
        // System Media apps
        'com.android.providers.media': '🎬',
        'com.android.gallery3d': '🖼️',
        'com.google.android.apps.photos': '📸',
        
        // System Communication
        'com.android.phone': '📞',
        'com.android.contacts': '👥',
        'com.android.messaging': '💬',
        'com.android.dialer': '📱',
        
        // System Framework
        'android.hardware.camera2': '⚙️',
        'com.android.systemui': '🔧',
        'android': '🤖'
    };
    
    // Try exact match first
    if (iconMap[packageName]) {
        return iconMap[packageName];
    }
    
    // Try partial matches for system apps
    const pkg = packageName.toLowerCase();
    if (pkg.includes('camera')) return '📷';
    if (pkg.includes('media') || pkg.includes('gallery')) return '🎬';
    if (pkg.includes('phone') || pkg.includes('dialer')) return '📞';
    if (pkg.includes('contact')) return '👥';
    if (pkg.includes('message')) return '💬';
    if (pkg.includes('system')) return '⚙️';
    
    return '🤖'; // Default system app icon
}

function setupEventListeners() {
    // Module toggle
    document.getElementById('moduleEnabled').addEventListener('change', async (e) => {
        appState.config.moduleEnabled = e.target.checked;
        await toggleModule(e.target.checked);
    });
    
    // Search functionality
    document.getElementById('appSearch').addEventListener('input', (e) => {
        appState.searchQuery = e.target.value.toLowerCase();
        renderAppList();
    });
    
    // Category tabs
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            e.target.classList.add('active');
            appState.currentCategory = e.target.dataset.category;
            renderAppList();
        });
    });
    
    // Select all button
    document.getElementById('selectAllBtn').addEventListener('click', () => {
        const visibleApps = getFilteredApps();
        const allSelected = visibleApps.every(app => appState.selectedApps.has(app.package));
        
        if (allSelected) {
            // Deselect all visible apps
            visibleApps.forEach(app => appState.selectedApps.delete(app.package));
        } else {
            // Select all visible apps
            visibleApps.forEach(app => appState.selectedApps.add(app.package));
        }
        
        renderAppList();
        updateHookedCount();
    });
    
    // Video file selection
    document.getElementById('videoFile').addEventListener('change', async (e) => {
        const file = e.target.files[0];
        if (file) {
            document.getElementById('fileName').textContent = file.name;
            
            try {
                showMessage('Uploading video file...', 'info');
                await WEBUI_API.uploadVideo(file);
                appState.config.videoPath = `/sdcard/MagicCam/${file.name}`;
                showMessage('Video uploaded successfully!', 'success');
            } catch (error) {
                console.error('Failed to upload video:', error);
                showMessage('Failed to upload video file', 'error');
            }
        }
    });
    
    // Settings
    document.getElementById('videoQuality').addEventListener('change', (e) => {
        appState.config.videoQuality = e.target.value;
    });
    
    document.getElementById('loopVideo').addEventListener('change', (e) => {
        appState.config.loopVideo = e.target.checked;
    });
    
    document.getElementById('detectionBypass').addEventListener('change', (e) => {
        appState.config.detectionBypass = e.target.checked;
    });
    
    document.getElementById('debugLogging').addEventListener('change', (e) => {
        appState.config.debugLogging = e.target.checked;
    });
    
    document.getElementById('hookMethod').addEventListener('change', (e) => {
        appState.config.hookMethod = e.target.value;
    });
    
    // Action buttons
    document.getElementById('saveSettings').addEventListener('click', saveConfiguration);
    document.getElementById('resetSettings').addEventListener('click', resetConfiguration);
    document.getElementById('exportConfig').addEventListener('click', exportConfiguration);
}

function getFilteredApps() {
    return appState.apps.filter(app => {
        // Category filter
        if (appState.currentCategory !== 'all' && app.category !== appState.currentCategory) {
            return false;
        }
        
        // Search filter
        if (appState.searchQuery && 
            !app.name.toLowerCase().includes(appState.searchQuery) &&
            !app.package.toLowerCase().includes(appState.searchQuery)) {
            return false;
        }
        
        return true;
    });
}

function renderAppList() {
    const appList = document.getElementById('appList');
    const filteredApps = getFilteredApps();
    
    if (appState.apps.length === 0) {
        // System loading failed - show error state
        appList.innerHTML = `
            <div class="error-state">
                <h3>❌ No System Apps Available</h3>
                <p>System app loading failed or no system apps found.</p>
                <p>WebUI requires system integration to function.</p>
            </div>
        `;
        return;
    }
    
    if (filteredApps.length === 0) {
        appList.innerHTML = `
            <div class="no-apps">
                <p>No system applications found matching your criteria.</p>
                <p>Try adjusting your search or category filter.</p>
            </div>
        `;
        return;
    }
    
    appList.innerHTML = filteredApps.map(app => `
        <div class="app-item ${appState.selectedApps.has(app.package) ? 'selected' : ''}" 
             data-package="${app.package}">
            <div class="app-checkbox"></div>
            <div class="app-icon">${app.icon}</div>
            <div class="app-info">
                <div class="app-name">${app.name}</div>
                <div class="app-package">${app.package}</div>
            </div>
            <div class="app-category">${app.category}</div>
            ${app.hasCamera ? '<div class="camera-badge">📷</div>' : ''}
            <div class="system-badge">SYS</div>
        </div>
    `).join('');
    
    // Add click listeners to app items
    appList.querySelectorAll('.app-item').forEach(item => {
        item.addEventListener('click', () => {
            const packageName = item.dataset.package;
            
            if (appState.selectedApps.has(packageName)) {
                appState.selectedApps.delete(packageName);
                item.classList.remove('selected');
            } else {
                appState.selectedApps.add(packageName);
                item.classList.add('selected');
            }
            
            updateHookedCount();
        });
    });
}

function updateHookedCount() {
    document.getElementById('hookedCount').textContent = appState.selectedApps.size;
}

async function toggleModule(enabled) {
    try {
        await WEBUI_API.toggleModule(enabled);
        updateModuleStatus({ enabled, virtualMode: enabled });
        showMessage(`Module ${enabled ? 'enabled' : 'disabled'} successfully`, 'success');
    } catch (error) {
        console.error('Failed to toggle module:', error);
        showMessage('Failed to toggle module', 'error');
        // Revert checkbox state
        document.getElementById('moduleEnabled').checked = !enabled;
    }
}

function updateModuleStatus(status) {
    const statusIndicator = document.getElementById('statusIndicator');
    const virtualStatus = document.getElementById('virtualStatus');
    
    if (status.enabled) {
        statusIndicator.querySelector('.status-dot').style.background = 'var(--success-color)';
        statusIndicator.querySelector('span').textContent = 'Module Active';
        virtualStatus.textContent = status.virtualMode ? 'Active' : 'Inactive';
        virtualStatus.className = status.virtualMode ? 'status-value status-active' : 'status-value';
    } else {
        statusIndicator.querySelector('.status-dot').style.background = 'var(--error-color)';
        statusIndicator.querySelector('span').textContent = 'Module Inactive';
        virtualStatus.textContent = 'Inactive';
        virtualStatus.className = 'status-value';
    }
}

async function loadConfiguration() {
    try {
        const config = await WEBUI_API.loadConfig();
        appState.config = { ...appState.config, ...config };
        
        // Update UI with loaded config
        document.getElementById('moduleEnabled').checked = appState.config.moduleEnabled;
        document.getElementById('videoQuality').value = appState.config.videoQuality;
        document.getElementById('loopVideo').checked = appState.config.loopVideo;
        document.getElementById('detectionBypass').checked = appState.config.detectionBypass;
        document.getElementById('debugLogging').checked = appState.config.debugLogging;
        document.getElementById('hookMethod').value = appState.config.hookMethod;
        
        // Load selected apps
        if (config.selectedApps) {
            appState.selectedApps = new Set(config.selectedApps);
            renderAppList();
            updateHookedCount();
        }
        
        // Update file name if video is set
        if (appState.config.videoPath) {
            const fileName = appState.config.videoPath.split('/').pop();
            document.getElementById('fileName').textContent = fileName;
        }
        
    } catch (error) {
        console.error('Failed to load configuration:', error);
        showMessage('Using default configuration', 'warning');
    }
}

async function saveConfiguration() {
    try {
        const config = {
            ...appState.config,
            selectedApps: Array.from(appState.selectedApps)
        };
        
        await WEBUI_API.saveConfig(config);
        showMessage('Configuration saved successfully!', 'success');
        
    } catch (error) {
        console.error('Failed to save configuration:', error);
        showMessage('Failed to save configuration', 'error');
    }
}

function resetConfiguration() {
    if (confirm('Are you sure you want to reset all settings to default?')) {
        appState.config = {
            moduleEnabled: true,
            virtualMode: true,
            videoPath: '',
            videoQuality: '1080p',
            loopVideo: true,
            detectionBypass: true,
            debugLogging: false,
            hookMethod: 'camera2ndk'
        };
        
        appState.selectedApps.clear();
        
        // Update UI
        document.getElementById('moduleEnabled').checked = true;
        document.getElementById('videoQuality').value = '1080p';
        document.getElementById('loopVideo').checked = true;
        document.getElementById('detectionBypass').checked = true;
        document.getElementById('debugLogging').checked = false;
        document.getElementById('hookMethod').value = 'camera2ndk';
        document.getElementById('fileName').textContent = 'No file selected';
        
        renderAppList();
        updateHookedCount();
        
        showMessage('Configuration reset to defaults', 'success');
    }
}

function exportConfiguration() {
    const config = {
        ...appState.config,
        selectedApps: Array.from(appState.selectedApps),
        exportDate: new Date().toISOString()
    };
    
    const blob = new Blob([JSON.stringify(config, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    
    const a = document.createElement('a');
    a.href = url;
    a.download = 'magic-cam-config.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    
    showMessage('Configuration exported successfully!', 'success');
}

function showMessage(text, type = 'info') {
    // Remove existing messages
    document.querySelectorAll('.message').forEach(msg => msg.remove());
    
    const message = document.createElement('div');
    message.className = `message message-${type}`;
    message.textContent = text;
    
    const container = document.querySelector('.main-content');
    container.insertBefore(message, container.firstChild);
    
    // Auto-remove after 5 seconds
    setTimeout(() => {
        if (message.parentNode) {
            message.remove();
        }
    }, 5000);
}

function updateUI() {
    updateHookedCount();
    
    // Update select all button text
    const selectAllBtn = document.getElementById('selectAllBtn');
    const visibleApps = getFilteredApps();
    const allSelected = visibleApps.length > 0 && visibleApps.every(app => appState.selectedApps.has(app.package));
    selectAllBtn.textContent = allSelected ? 'Deselect All' : 'Select All';
}

// Add CSS for error states
const additionalCSS = `
.error-state {
    text-align: center;
    padding: 40px 20px;
    background: rgba(239, 68, 68, 0.1);
    border: 2px dashed var(--error-color);
    border-radius: var(--radius);
    color: var(--error-color);
}

.error-state h3 {
    margin-bottom: 16px;
    font-size: 18px;
}

.error-state p {
    margin-bottom: 8px;
    color: var(--text-muted);
}

.no-apps {
    text-align: center;
    padding: 40px 20px;
    color: var(--text-muted);
}

.camera-badge, .system-badge {
    font-size: 10px;
    padding: 2px 6px;
    border-radius: 4px;
    font-weight: 500;
    text-transform: uppercase;
}

.camera-badge {
    background: var(--success-color);
    color: white;
}

.system-badge {
    background: var(--primary-color);
    color: white;
}

.message-info {
    background: rgba(99, 102, 241, 0.1);
    color: var(--primary-color);
    border: 1px solid rgba(99, 102, 241, 0.2);
}

.message-warning {
    background: rgba(245, 158, 11, 0.1);
    color: var(--warning-color);
    border: 1px solid rgba(245, 158, 11, 0.2);
}
`;

// Inject additional CSS
const style = document.createElement('style');
style.textContent = additionalCSS;
document.head.appendChild(style);