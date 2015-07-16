import sys

svg_head = "<?xml version=\"1.0\" standalone=\"no\"?>\n<svg version=\"1.1\" "

kernel_color = {"atmi_spotrf_kernel_cpu_wrapper":"#000000", "atmi_strsm_kernel_cpu_wrapper":"#cc0000", "atmi_ssyrk_kernel_cpu_wrapper":"#0000cc", "atmi_sgemm_kernel_cpu_wrapper":"#cccc00"}

if __name__ == '__main__':
    trace_fname = "spotrf.svg"
    ofile = open(trace_fname, 'w')
    pfname = sys.argv[1]    

    loc_x = 0
    loc_y = 0
    height = 30
    width = 0
    time_start = 0
    x_offset = 100
    y_offset = 20
    unit = 100000
    unit_width = 5
    stroke = " stroke=\"#000000\" stroke-width=\"1\""
    stroke_width = 0
    tid = 0
    nb_agents = 999
    max_loc_x = 0
    svg_str = ""
 
    while (tid < nb_agents):
        line_ct = 0
        profiling_fname = pfname + "_" + str(tid)
        ifile = open(profiling_fname, 'r')
        for line in ifile:
            line_str = line.split()
            line_ct = line_ct +1
            if (line_ct == 1):
                tid = int(line_str[0])
                nb_agents = int(line_str[1])
                continue
            if (line_str[0] == "__sync_kernel_wrapper"):
                continue
            if (time_start == 0):
                time_start = int(line_str[1])
            loc_x = int(line_str[1]) - time_start   
            loc_y = y_offset + height * tid  
            width = int(line_str[2]) - int(line_str[1])
            loc_x = loc_x / 100000 * unit_width + x_offset
            width = width / 100000 * unit_width
            rect_str = "<rect x=\"" + str(loc_x) + "\" y=\"" + str(loc_y) + "\" width=\"" + str(width) + "\" height=\"" + str(height) + "\" fill=\"" + kernel_color[line_str[0]] + "\"" + stroke + "/>"
            svg_str = svg_str + "\n" + rect_str
            if (loc_x > max_loc_x):
                max_loc_x = loc_x
        tid = tid + 1
    
    max_loc_x += 100
    svg_head += "width=\"" + str(max_loc_x)  + "\" height=\"800\" viewBox=\"0 0 " + str(max_loc_x)  + " 800\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
    svg_head += svg_str
    svg_head += "\n</svg>\n"
    ofile.write(svg_head);
    print svg_head


