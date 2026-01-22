/*
 * VARTA - Acoustic Drone Detector Enclosure
 * OpenSCAD parametric design
 * 
 * Generate STL files:
 *   openscad -o varta_body.stl -D 'part="body"' enclosure.scad
 *   openscad -o varta_lid.stl -D 'part="lid"' enclosure.scad
 *   openscad -o varta_clip.stl -D 'part="clip"' enclosure.scad
 *   openscad -o varta_assembly.stl -D 'part="assembly"' enclosure.scad
 */

// Part selection
part = "assembly";  // "body", "lid", "clip", "assembly"

// =============================================================================
// PARAMETERS
// =============================================================================

// Overall dimensions
body_length = 140;      // mm
body_width = 80;        // mm
body_height = 35;       // mm
lid_height = 10;        // mm
wall_thickness = 2.5;   // mm
corner_radius = 5;      // mm

// Microphone array
mic_spacing = 50;       // mm between adjacent mics
mic_hole_dia = 8;       // mm (INMP441 breakout board)
mic_mount_dia = 12;     // mm

// Display cutout (OLED)
oled_width = 28;        // mm
oled_height = 15;       // mm
oled_offset_x = 30;     // mm from center
oled_offset_y = 0;      // mm from center

// LED ring cutout
led_ring_dia = 32;      // mm (8-LED ring)
led_ring_offset = -35;  // mm from center (toward rear)

// Button
button_dia = 8;         // mm
button_offset_x = 50;   // mm from center
button_offset_y = -20;  // mm from center

// USB port cutout
usb_width = 12;         // mm
usb_height = 7;         // mm

// Battery compartment
battery_length = 70;    // mm (2x 18650)
battery_width = 40;     // mm
battery_height = 20;    // mm

// Screw bosses
screw_boss_dia = 8;     // mm
screw_hole_dia = 3;     // mm (M3)
screw_depth = 10;       // mm

// Tolerances
fit_tolerance = 0.3;    // mm

// =============================================================================
// MODULES
// =============================================================================

module rounded_box(length, width, height, radius) {
    hull() {
        translate([radius, radius, 0])
            cylinder(r=radius, h=height, $fn=32);
        translate([length-radius, radius, 0])
            cylinder(r=radius, h=height, $fn=32);
        translate([radius, width-radius, 0])
            cylinder(r=radius, h=height, $fn=32);
        translate([length-radius, width-radius, 0])
            cylinder(r=radius, h=height, $fn=32);
    }
}

module screw_boss(height, outer_dia, hole_dia) {
    difference() {
        cylinder(d=outer_dia, h=height, $fn=24);
        translate([0, 0, -0.1])
            cylinder(d=hole_dia, h=height+0.2, $fn=16);
    }
}

module mic_port() {
    // Acoustic port with protective mesh holder
    union() {
        // Main hole
        cylinder(d=mic_hole_dia, h=wall_thickness*3, center=true, $fn=32);
        // Countersink for mesh
        translate([0, 0, wall_thickness/2])
            cylinder(d=mic_mount_dia, h=2, $fn=32);
    }
}

// =============================================================================
// BODY
// =============================================================================

module body() {
    difference() {
        // Outer shell
        rounded_box(body_length, body_width, body_height, corner_radius);
        
        // Inner cavity
        translate([wall_thickness, wall_thickness, wall_thickness])
            rounded_box(
                body_length - 2*wall_thickness,
                body_width - 2*wall_thickness,
                body_height,  // Open top
                corner_radius - wall_thickness
            );
        
        // USB port cutout (rear)
        translate([body_length - wall_thickness - 1, body_width/2 - usb_width/2, 5])
            cube([wall_thickness + 2, usb_width, usb_height]);
        
        // Ventilation slots (sides)
        for (y = [15, 25, 35, 45, 55, 65]) {
            // Left side
            translate([-1, y, body_height - 10])
                cube([wall_thickness + 2, 3, 6]);
            // Right side
            translate([body_length - wall_thickness - 1, y, body_height - 10])
                cube([wall_thickness + 2, 3, 6]);
        }
    }
    
    // Screw bosses (4 corners)
    boss_inset = corner_radius + 2;
    translate([boss_inset, boss_inset, wall_thickness])
        screw_boss(body_height - wall_thickness - 2, screw_boss_dia, screw_hole_dia);
    translate([body_length - boss_inset, boss_inset, wall_thickness])
        screw_boss(body_height - wall_thickness - 2, screw_boss_dia, screw_hole_dia);
    translate([boss_inset, body_width - boss_inset, wall_thickness])
        screw_boss(body_height - wall_thickness - 2, screw_boss_dia, screw_hole_dia);
    translate([body_length - boss_inset, body_width - boss_inset, wall_thickness])
        screw_boss(body_height - wall_thickness - 2, screw_boss_dia, screw_hole_dia);
    
    // Battery holder rails
    translate([body_length/2 - battery_length/2, body_width/2 - battery_width/2 - 5, wall_thickness])
        cube([battery_length, 3, 5]);
    translate([body_length/2 - battery_length/2, body_width/2 + battery_width/2 + 2, wall_thickness])
        cube([battery_length, 3, 5]);
    
    // PCB standoffs
    standoff_height = 5;
    standoff_dia = 5;
    standoff_hole = 2;
    
    // ESP32-S3 mounting points (adjust based on your PCB)
    for (pos = [
        [30, 20],
        [30, 60],
        [90, 20],
        [90, 60]
    ]) {
        translate([pos[0], pos[1], wall_thickness])
            screw_boss(standoff_height, standoff_dia, standoff_hole);
    }
}

// =============================================================================
// LID
// =============================================================================

module lid() {
    center_x = body_length / 2;
    center_y = body_width / 2;
    
    difference() {
        union() {
            // Main lid
            rounded_box(body_length, body_width, lid_height, corner_radius);
            
            // Inner lip for sealing
            translate([wall_thickness + fit_tolerance, wall_thickness + fit_tolerance, -3])
                rounded_box(
                    body_length - 2*(wall_thickness + fit_tolerance),
                    body_width - 2*(wall_thickness + fit_tolerance),
                    3,
                    corner_radius - wall_thickness - fit_tolerance
                );
        }
        
        // Microphone ports (square array, 50mm spacing)
        // M1: Front Left
        translate([center_x - mic_spacing/2, center_y + mic_spacing/2, 0])
            mic_port();
        // M2: Front Right  
        translate([center_x + mic_spacing/2, center_y + mic_spacing/2, 0])
            mic_port();
        // M3: Rear Right
        translate([center_x + mic_spacing/2, center_y - mic_spacing/2, 0])
            mic_port();
        // M4: Rear Left
        translate([center_x - mic_spacing/2, center_y - mic_spacing/2, 0])
            mic_port();
        
        // OLED display window
        translate([center_x + oled_offset_x - oled_width/2, 
                   center_y + oled_offset_y - oled_height/2, -1])
            cube([oled_width, oled_height, lid_height + 2]);
        
        // LED ring window
        translate([center_x + led_ring_offset, center_y, -1])
            cylinder(d=led_ring_dia, h=lid_height + 2, $fn=48);
        
        // Button hole
        translate([center_x + button_offset_x, center_y + button_offset_y, -1])
            cylinder(d=button_dia, h=lid_height + 2, $fn=24);
        
        // Screw holes (matching body bosses)
        boss_inset = corner_radius + 2;
        for (pos = [
            [boss_inset, boss_inset],
            [body_length - boss_inset, boss_inset],
            [boss_inset, body_width - boss_inset],
            [body_length - boss_inset, body_width - boss_inset]
        ]) {
            translate([pos[0], pos[1], -1])
                cylinder(d=screw_hole_dia + 0.5, h=lid_height + 2, $fn=16);
            // Countersink for screw head
            translate([pos[0], pos[1], lid_height - 2])
                cylinder(d=6, h=3, $fn=24);
        }
        
        // Direction indicator arrow (front marker)
        translate([center_x, body_width - 8, lid_height - 0.5])
            linear_extrude(1)
                polygon([[-4, 0], [4, 0], [0, 6]]);
    }
    
    // Mic mounting rings (raised)
    for (pos = [
        [center_x - mic_spacing/2, center_y + mic_spacing/2],
        [center_x + mic_spacing/2, center_y + mic_spacing/2],
        [center_x + mic_spacing/2, center_y - mic_spacing/2],
        [center_x - mic_spacing/2, center_y - mic_spacing/2]
    ]) {
        translate([pos[0], pos[1], lid_height])
            difference() {
                cylinder(d=mic_mount_dia + 2, h=2, $fn=32);
                translate([0, 0, -0.1])
                    cylinder(d=mic_mount_dia, h=2.2, $fn=32);
            }
    }
}

// =============================================================================
// BELT/MOLLE CLIP
// =============================================================================

module clip() {
    clip_width = 40;
    clip_height = 50;
    clip_thickness = 3;
    belt_gap = 6;  // For standard belt or MOLLE
    
    // Main plate
    difference() {
        cube([clip_width, clip_thickness + 5, clip_height]);
        
        // Belt slot
        translate([-1, clip_thickness, 10])
            cube([clip_width + 2, belt_gap, 30]);
    }
    
    // Mounting tabs (screw to body)
    tab_spacing = 30;
    translate([(clip_width - tab_spacing)/2, 0, clip_height])
        rotate([-90, 0, 0])
            difference() {
                cube([tab_spacing, 15, clip_thickness]);
                translate([5, 7.5, -1])
                    cylinder(d=screw_hole_dia, h=clip_thickness + 2, $fn=16);
                translate([tab_spacing - 5, 7.5, -1])
                    cylinder(d=screw_hole_dia, h=clip_thickness + 2, $fn=16);
            }
    
    // Retention hook
    translate([0, clip_thickness, 0])
        cube([clip_width, 2, 8]);
    translate([0, clip_thickness + belt_gap - 2, 0])
        cube([clip_width, 2, 8]);
}

// =============================================================================
// RENDER
// =============================================================================

if (part == "body") {
    body();
} else if (part == "lid") {
    lid();
} else if (part == "clip") {
    clip();
} else if (part == "assembly") {
    // Show assembled view
    color("DarkGray") body();
    color("Gray") translate([0, 0, body_height]) lid();
    color("Black") translate([body_length + 10, 20, 0]) clip();
}
