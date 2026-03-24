/**
 * Game Server Web Interface
 * Main JavaScript file
 */

// ============================================
// UTILITY FUNCTIONS
// ============================================

/**
 * Show alert message
 * @param {string} message - Message to display
 * @param {string} type - 'success' or 'error'
 */
function showAlert(message, type = 'error') {
    const alert = $('#alert-message');
    
    // Remove previous classes
    alert.removeClass('error success hidden');
    
    // Add new classes
    alert.addClass(type);
    alert.text(message);
    alert.fadeIn(300);
    
    // Auto hide after 5 seconds
    setTimeout(() => {
        alert.fadeOut(300, () => {
            alert.addClass('hidden');
        });
    }, 5000);
}

/**
 * Update time display
 */
function updateTime() {
    const now = new Date();
    const timeString = now.toLocaleTimeString();
    $('#current-time').text(timeString);
}

/**
 * Load maps from API
 */
function loadMaps() {
    console.log('Loading maps...');

    $.ajax({
        url: '/api/v1/maps',
        method: 'GET',
        contentType: 'application/json',
        dataType: 'json'
    })
    .done(function(maps) {
        console.log('Maps loaded:', maps);
        const mapSelect = $('#map-select');
        mapSelect.empty();
        
        if (maps.length === 0) {
            mapSelect.append('<option value="" disabled>No maps available</option>');
            return;
        }
        
        mapSelect.append('<option value="" disabled selected>Select a map</option>');
        
        $.each(maps, function(index, map) {
            mapSelect.append(
                $('<option></option>')
                    .attr('value', map.id)
                    .text(map.name)
            );
        });
    })
    .fail(function(jqXHR, textStatus, errorThrown) {
        console.error('Failed to load maps:', textStatus, errorThrown);
        showAlert('Cannot load maps from server', 'error');
        $('#map-select').html('<option value="" disabled>Failed to load maps</option>');
    });
}

/**
 * Check for saved player data
 */
function checkSavedData() {
    const savedName = localStorage.getItem('playerName');
    if (savedName) {
        $('#player-name').val(savedName);
        console.log('Restored saved name:', savedName);
    }
}

/**
 * Handle form submission
 */
function handleLoginFormSubmit(e) {
    e.preventDefault();
    
    const playerName = $('#player-name').val().trim();
    const mapId = $('#map-select').val();
    
    // Validation
    if (!playerName) {
        showAlert('Please enter your name', 'error');
        return false;
    }
    
    if (!mapId) {
        showAlert('Please select a map', 'error');
        return false;
    }
    
    // Disable form
    const submitBtn = $('#login-btn');
    const originalHtml = submitBtn.html();
    submitBtn.prop('disabled', true);
    submitBtn.html('<span class="spinner"></span> Joining game...');
    
    // Send request
    $.ajax({
        type: "POST",
        url: '/api/v1/game/join',
        dataType: 'json',
        contentType: "application/json",
        data: JSON.stringify({
            userName: playerName,
            mapId: mapId
        })
    })
    .done(function(response) {
        console.log('Join successful:', response);
    
    // Get form values again to ensure they're available
    const playerName = $('#player-name').val().trim();
    const mapId = $('#map-select').val();
    
    // Set COOKIES (required by game.html)
    document.cookie = `authToken=${response.authToken}; path=/`;
    document.cookie = `playerId=${response.playerId}; path=/`;
    document.cookie = `mapId=${mapId}; path=/`;
    
    // Optional: Also save in localStorage for your UI
    localStorage.setItem('authToken', response.authToken);
    localStorage.setItem('playerId', response.playerId);
    localStorage.setItem('mapId', mapId);
    localStorage.setItem('playerName', playerName);
    
    // Save token in the Auth Token field
    if (response.authToken) {
        $('#auth-token').val(response.authToken);
    }
    
    showAlert('Successfully joined the game! Redirecting...', 'success');
    
    // Redirect after delay
    setTimeout(function() {
        window.location.href = '/game.html';
    }, 500);

    })
    .fail(function(xhr) {
        console.error('Join failed:', xhr);
        
        // Restore button
        submitBtn.prop('disabled', false);
        submitBtn.html(originalHtml);
        
        let errorMessage = 'Server error. Please try again.';
        
        if (xhr.responseJSON && xhr.responseJSON.message) {
            errorMessage = xhr.responseJSON.message;
        } else if (xhr.status === 404) {
            errorMessage = 'Map not found';
        } else if (xhr.status === 401 || xhr.status === 400) {
            errorMessage = 'Invalid request';
        }
        
        showAlert(errorMessage, 'error');
    });
    
    return false;
}

// ============================================
// INITIALIZATION
// ============================================

$(document).ready(function() {
    console.log('Game Server Web Interface loaded');
    
    // Initialize components
    try {
        loadMaps();
        checkSavedData();
        updateTime();
        setupMapIdInput();
        setupApiLinks();
        setupApiEndpoints();
    } catch (error) {
        console.error("Something broke but other stuff still works:", error);
    }
    
    // Set up event handlers
    $('#player-login-form').on('submit', handleLoginFormSubmit);
    
    // Set up tick preview update
    $('#tick-time-delta').on('input', updateTickPreview);
    
    // Set default auth token from localStorage
    const savedAuthToken = localStorage.getItem('authToken');
    if (savedAuthToken) {
        $('#auth-token').val(savedAuthToken);
    }
    
    // Set up intervals
    setInterval(updateTime, 1000);
    
    // Log initialization complete
    console.log('Initialization complete');
});

// ============================================
// MAP ID INPUT HANDLING
// ============================================

/**
 * Handle Enter key in map ID input
 */
function handleMapIdKeydown(event) {
    if (event.key === 'Enter') {
        event.preventDefault();
        goToMapById();
    }
}

/**
 * Navigate to specific map by ID
 */
function goToMapById() {
    const mapId = $('#map-id-input').val().trim();
    
    if (!mapId) {
        showAlert('Please enter a map ID', 'error');
        return;
    }
    
    // Build the full URL
    const url = `/api/v1/maps/${encodeURIComponent(mapId)}`;
    
    // Use AJAX request instead of direct navigation
    $.ajax({
        url: url,
        method: 'GET',
        contentType: 'application/json',
        dataType: 'json'
    })
    .done(function(data) {
        // Display the map data in large alert
        const jsonString = JSON.stringify(data, null, 2);
        showLargeAlert(`Map Data (ID: ${mapId})`, jsonString);
        
        showAlert('Map data loaded successfully', 'success');
    })
    .fail(function(xhr) {
        console.error('Failed to load map:', xhr);
        
        let errorMessage = 'Failed to load map';
        if (xhr.status === 404) {
            errorMessage = `Map with ID "${mapId}" not found`;
        } else if (xhr.responseJSON && xhr.responseJSON.message) {
            errorMessage = xhr.responseJSON.message;
        }
        
        showAlert(errorMessage, 'error');
    });
}

/**
 * Set up map ID input handlers
 */
function setupMapIdInput() {
    // Set up button click handler
    $('#go-to-map-btn').on('click', goToMapById);
    
    // Auto-focus the input
    $('#map-id-input').focus();
}

/**
 * Set up API link handlers
 */
function setupApiLinks() {
    // Handle "Browse all maps" link
    $('#browse-maps-link').on('click', function(e) {
        e.preventDefault();
        
        $.ajax({
            url: '/api/v1/maps',
            method: 'GET',
            contentType: 'application/json',
            dataType: 'json'
        })
        .done(function(data) {
            // Display all maps data in large alert
            const jsonString = JSON.stringify(data, null, 2);
            const mapCount = Array.isArray(data) ? data.length : 0;
            showLargeAlert(`All Maps (${mapCount} maps found)`, jsonString);
        })
        .fail(function(xhr) {
            showAlert('Failed to load maps data', 'error');
        });
    });
}

// ============================================
// PUBLIC API (if needed by other scripts)
// ============================================

window.GameServer = window.GameServer || {};

window.GameServer.Utils = {
    showAlert,
    updateTime,
    loadMaps
};

// ============================================
// GAME API ENDPOINTS HANDLING
// ============================================

/**
 * Initialize API endpoint handlers
 */
function setupApiEndpoints() {
    // Set up test buttons in table
    $('.table-test-btn').on('click', function(e) {
        e.stopPropagation();
        const endpoint = $(this).data('endpoint');
        testEndpoint(endpoint);
    });
    
    // Set up response buttons in table
    $('.table-response-btn').on('click', function(e) {
        e.stopPropagation();
        const endpoint = $(this).data('endpoint');
        showEndpointResponse(endpoint);
    });
    
    // Set up action endpoint send button
    $('.send-action-btn').on('click', function(e) {
        e.stopPropagation();
        sendPlayerAction();
    });
    
    // Set up tick endpoint send button
    $('.send-tick-btn').on('click', function(e) {
        e.stopPropagation();
        sendGameTick();
    });
    
    // Update move preview when direction changes
    $('#action-move').on('change', updateMovePreview);
    
    // Update tick preview when time delta changes
    $('#tick-time-delta').on('input', function() {
        updateTickPreview();
        validateTickInput(this);
    });
    $('#tick-time-delta').on('change', updateTickPreview);
    
    // Initialize the move preview
    updateMovePreview();
    
    // Initialize the tick preview
    updateTickPreview();
    
    // Set up response container buttons
    $('#proceed-to-game-btn').on('click', function() {
        window.location.href = '/game.html';
    });
    
    $('#close-response-btn').on('click', function() {
        $('#api-response-container').addClass('hidden');
    });
}

// Add new validation function
function validateTickInput(inputElement) {
    const value = $(inputElement).val();
    
    // Remove any non-digit characters
    const cleaned = value.replace(/[^\d]/g, '');
    
    // If empty or invalid, set to default
    if (!cleaned || parseInt(cleaned) <= 0) {
        $(inputElement).val('1000');
        updateTickPreview();
        return;
    }
    
    // Update with cleaned value if different
    if (cleaned !== value) {
        $(inputElement).val(cleaned);
    }
}

// Add new function to update tick preview with better validation
function updateTickPreview() {
    const timeDelta = $('#tick-time-delta').val();
    const timeDeltaInt = parseInt(timeDelta);
    
    // Use default if invalid
    let validTimeDelta = 1000;
    if (!isNaN(timeDeltaInt) && timeDeltaInt > 0) {
        validTimeDelta = timeDeltaInt;
    }
    
    // Update input if needed (only if completely invalid)
    if (isNaN(timeDeltaInt) || timeDeltaInt <= 0) {
        $('#tick-time-delta').val(validTimeDelta.toString());
    }
    
    const requestBody = JSON.stringify({ timeDelta: validTimeDelta }, null, 2);
    $('#tick-preview').text(requestBody);
}

/**
 * Test API endpoint - SIMPLIFIED
 */
function testEndpoint(endpoint) {
    const authToken = $('#auth-token').val().trim();
    
    let url, method, data;
    
    switch(endpoint) {
        case 'join':
            const playerName = $('#player-name').val().trim();
            const mapId = $('#map-select').val();
            
            if (!playerName || !mapId) {
                showAlert('Please enter player name and select map first', 'error');
                return;
            }
            
            url = '/api/v1/game/join';
            method = 'POST';
            data = JSON.stringify({ userName: playerName, mapId: mapId });
            break;
            
        case 'players':
            url = '/api/v1/game/players';
            method = 'GET';
            break;
            
        case 'state':
            url = '/api/v1/game/state';
            method = 'GET';
            break;
            
        case 'action':
            alert('Action Endpoint Test: Use the "Send Action" button for actual API call');
            return;
            
        case 'tick':
            alert('Tick Endpoint Test: Use the "Send Tick" button for actual API call');
            return;
    }
    
    showAlert(`Testing ${endpoint} endpoint...`, 'success');
    
    const headers = { 'Content-Type': 'application/json' };
    if (authToken && endpoint !== 'join') {
        headers['Authorization'] = `Bearer ${authToken}`;
    }
    
    $.ajax({ url, method, headers, data, dataType: 'json' })
    .done(function(response) {
        if (endpoint === 'join') {
            if (response.authToken) {
                $('#auth-token').val(response.authToken);
                localStorage.setItem('authToken', response.authToken);
                localStorage.setItem('playerId', response.playerId);
                localStorage.setItem('playerName', $('#player-name').val().trim());
            }
            showResponseInContainer(response);
        } else {
            const jsonString = JSON.stringify(response, null, 2);
            showLargeAlert(`${endpoint.toUpperCase()} Response`, jsonString);
        }
        showAlert(`${endpoint} endpoint test successful!`, 'success');
    })
    .fail(function(xhr) {
        const errorMsg = xhr.responseJSON?.message || `Error ${xhr.status}`;
        showAlert(errorMsg, 'error');
    });
}

/**
 * Test tick endpoint - SIMPLIFIED
 */
function testTickEndpoint() {
    alert('Tick Endpoint Test: Use the "Send Tick" button for actual API call');
    showAlert('Use "Send Tick" button for actual API call', 'success');
}

/**
 * Send game tick - SIMPLIFIED
 */
function sendGameTick() {
    const timeDelta = $('#tick-time-delta').val() || 1000;
    
    $.ajax({
        url: '/api/v1/game/tick',
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        data: JSON.stringify({ timeDelta: parseInt(timeDelta) }),
        dataType: 'json'
    })
    .done(function(response, textStatus, xhr) {
        // Simple response display
        const jsonString = JSON.stringify(response, null, 2);
        alert(`Tick Response (${xhr.status}):\n\n${jsonString}`);
        showAlert('Tick sent successfully!', 'success');
    })
    .fail(function(xhr) {
        // Simple error display
        const errorMsg = xhr.responseJSON?.message || `Error ${xhr.status}`;
        alert(`Tick Error (${xhr.status}):\n\n${errorMsg}`);
        showAlert('Failed to send tick', 'error');
    });
}

/**
 * Show endpoint response in the response container
 */
function showEndpointResponse(endpoint) {
    if (endpoint === 'join') {
        // For join, show sample response
        const sampleResponse = {
            authToken: "sample-auth-token-12345",
            playerId: "player-123",
            message: "Successfully joined the game",
            position: { x: 10, y: 10 },
            health: 100,
            inventory: []
        };
        showResponseInContainer(sampleResponse, true);
    } else if (endpoint === 'tick') {
        // For tick, show sample response
        const sampleResponse = {
            message: "Game time advanced",
            currentTime: Date.now()
        };
        showResponseInContainer(sampleResponse, true);
    }
}

/**
 * Show response in the response container
 */
function showResponseInContainer(response, isSample = false) {
    const jsonString = JSON.stringify(response, null, 2);
    $('#api-response').text(jsonString);
    
    // Show the container
    $('#api-response-container').removeClass('hidden');
    
    // Scroll to container
    $('#api-response-container')[0].scrollIntoView({ behavior: 'smooth' });
    
    if (isSample) {
        $('#proceed-to-game-btn').addClass('hidden');
        showAlert('This is a sample response. Use the Test button for real API call.', 'success');
    } else {
        $('#proceed-to-game-btn').removeClass('hidden');
    }
}

function showLargeAlert(title, message) {
    // Create a custom dialog instead of using alert()
    const dialogId = 'custom-alert-dialog';
    const overlayId = 'custom-alert-overlay';
    
    // Remove existing dialog and overlay if any
    $(`#${dialogId}`).remove();
    $(`#${overlayId}`).remove();
    
    // Create dialog container
    const dialog = $('<div></div>')
        .attr('id', dialogId)
        .addClass('custom-alert-dialog')
        .css({
            'position': 'fixed',
            'top': '50%',
            'left': '50%',
            'transform': 'translate(-50%, -50%)',
            'background': 'white',
            'padding': '20px',
            'border-radius': '8px',
            'box-shadow': '0 5px 30px rgba(0,0,0,0.3)',
            'z-index': '10000',
            'max-width': '90vw',
            'max-height': '80vh',
            'width': '800px',  // Wider
            'height': '600px',  // Taller - 2x bigger
            'display': 'flex',
            'flex-direction': 'column'
        });
    
    // Create title
    const titleEl = $('<h3></h3>')
        .text(title)
        .css({
            'margin': '0 0 15px 0',
            'padding': '0',
            'color': '#333',
            'font-size': '18px',
            'border-bottom': '1px solid #eee',
            'padding-bottom': '10px'
        });
    
    // Create message area with scroll
    const messageEl = $('<pre></pre>')
        .text(message)
        .css({
            'flex': '1',
            'margin': '0',
            'padding': '15px',
            'background': '#f5f5f5',
            'border-radius': '4px',
            'overflow': 'auto',
            'font-family': 'monospace',
            'font-size': '12px',
            'white-space': 'pre-wrap',
            'word-wrap': 'break-word'
        });
    
    // Create close button
    const closeButton = $('<button></button>')
        .text('Close')
        .css({
            'margin-top': '15px',
            'padding': '10px 20px',
            'background': '#007bff',
            'color': 'white',
            'border': 'none',
            'border-radius': '4px',
            'cursor': 'pointer',
            'align-self': 'flex-end'
        })
        .on('click', function() {
            // Remove both dialog AND overlay when close button is clicked
            dialog.remove();
            overlay.remove();
            $(document).off('keydown.customAlert');
        });
    
    // Assemble dialog
    dialog.append(titleEl);
    dialog.append(messageEl);
    dialog.append(closeButton);
    
    // Add overlay
    const overlay = $('<div></div>')
        .attr('id', overlayId)
        .addClass('custom-alert-overlay')
        .css({
            'position': 'fixed',
            'top': '0',
            'left': '0',
            'right': '0',
            'bottom': '0',
            'background': 'rgba(0,0,0,0.5)',
            'z-index': '9999'
        })
        .on('click', function() {
            // Remove both overlay AND dialog when overlay is clicked
            $(this).remove();
            dialog.remove();
            $(document).off('keydown.customAlert');
        });
    
    // Add to body - overlay first, then dialog
    $('body').append(overlay);
    $('body').append(dialog);
    
    // Auto-focus close button
    closeButton.focus();
    
    // Allow closing with Escape key
    $(document).on('keydown.customAlert', function(e) {
        if (e.key === 'Escape') {
            dialog.remove();
            overlay.remove();
            $(document).off('keydown.customAlert');
        }
    });
}


/**
 * Update move direction preview
 */
function updateMovePreview() {
    const moveValue = $('#action-move').val();
    const requestBody = JSON.stringify({ move: moveValue }, null, 2);
    $('#request-preview').text(requestBody);
    $('#action-data').val(requestBody);
}

/**
 * Send player action - SIMPLIFIED
 */
function sendPlayerAction() {
    const authToken = $('#auth-token').val().trim();
    const moveValue = $('#action-move').val();
    
    if (!authToken) {
        showAlert('Please enter auth token first', 'error');
        return;
    }
    
    $.ajax({
        url: '/api/v1/game/player/action',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${authToken}`
        },
        data: JSON.stringify({ move: moveValue }),
        dataType: 'json'
    })
    .done(function(response, textStatus, xhr) {
        // Simple response display
        const jsonString = JSON.stringify(response, null, 2);
        alert(`Action Response (${xhr.status}):\n\n${jsonString}`);
        showAlert('Action sent successfully!', 'success');
    })
    .fail(function(xhr) {
        // Simple error display
        const errorMsg = xhr.responseJSON?.message || `Error ${xhr.status}`;
        alert(`Action Error (${xhr.status}):\n\n${errorMsg}`);
        showAlert('Failed to send action', 'error');
    });
}

/**
 * Test action endpoint - SIMPLIFIED
 */
function testActionEndpoint() {
    const authToken = $('#auth-token').val().trim();
    
    if (!authToken) {
        showAlert('Please enter auth token first', 'error');
        return;
    }
    
    alert('Action Endpoint Test: Use the "Send Action" button for actual API call');
    showAlert('Check the console for actual API call', 'success');
}


