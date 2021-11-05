"use strict";

var Module = {};


(function() {
    var global;
    var all_list;
    var current_data;
    var colors;

    function hsv_to_rgb(h,s,v) {
        var h_i = Math.floor(h*6);
        var f = h*6 - h_i;
        var p = v * (1 - s);
        var q = v * (1 - f*s);
        var t = v * (1 - (1 - f) * s);
        var ret;

        switch (h_i) {
        case 0: { ret = [v, t, p]; break; }
        case 1: { ret = [q, v, p]; break; }
        case 2: { ret = [p, v, t]; break; }
        case 3: { ret = [p, q, v]; break; }
        case 4: { ret = [t, p, v]; break; }
        default: { ret = [v, p, q]; break; }
        }

        return [Math.floor(ret[0]*256),
                Math.floor(ret[1]*256),
                Math.floor(ret[2]*256)];
    }

    function gen_colors() {
        var nelem = current_data.length;
        colors = new Array(nelem);

        for (var i=0; i<nelem; i++) {
            var h = (1.0/nelem) * i;
            var s = 0.4;
            var v = 1.0;

            var rgb = hsv_to_rgb(h,s,v);

            colors[i] = "rgb("+rgb[0]+","+rgb[1]+","+rgb[2]+")";
        }
    }

    function start() {
        fetch('./combined.json').then(response => response.json()).then(data => {
            global = new Module.GlobalState();
            all_list = Module.get_all_benchmark_list();
            current_data = data;

            gen_colors();

            update_html();
        });
    }

    function insert_text_p(elem, text) {
        var t = document.createTextNode(text);
        var p = document.createElement('p');
        p.appendChild(t);
        elem.appendChild(p);
    }

    function insert_text(elem, text) {
        var t = document.createTextNode(text);
        elem.appendChild(t);
    }

    function update_html() {
        var viewer_base = document.getElementById('viewer');
        var child = viewer_base.lastElementChild;
        while (child) {
            viewer_base.removeChild(child);
            child = viewer_base.lastElementChild;
        }

        var viewer = document.createElement('div');
        viewer_base.appendChild(viewer);

        {
            var machines = document.createElement('div');
            machines.setAttribute('id','machines');

            insert_text_p(machines, 'select base target');
            var list = document.createElement('ul');
            for (var i=0; i<current_data.length; i++) {
                var li = document.createElement('li');
                var cb = document.createElement('input');
                cb.setAttribute('type','radio');
                cb.setAttribute('name', 'base_cpu');
                if(i==0) {
                    cb.setAttribute('checked','checked');
                }

                li.style.backgroundColor = colors[i];
                li.appendChild(cb);
                insert_text(li, current_data[i].sysinfo.cpuid + '/' + current_data[i].sysinfo.os);

                list.appendChild(li);
            }

            machines.appendChild(list);


            insert_text_p(machines, 'select compare targets');
            var list = document.createElement('ul');
            for (var i=0; i<current_data.length; i++) {
                var li = document.createElement('li');
                var cb = document.createElement('input');
                cb.setAttribute('type','checkbox');
                cb.setAttribute('name', 'target_cpu'+i);
                cb.setAttribute('checked','checked');

                li.style.backgroundColor = colors[i];
                li.appendChild(cb);
                insert_text(li, current_data[i].sysinfo.cpuid + '/' + current_data[i].sysinfo.os);

                list.appendChild(li);
            }

            machines.appendChild(list);
            viewer.appendChild(machines);
        }

        var results = document.createElement('div');
        results.id = 'results'

        for (var i=0; i<all_list.size(); i++) {
            var det = document.createElement('details');
            var sum = document.createElement('summary');
            var content = document.createElement('div');

            var t = document.createTextNode(all_list.get(i).name);

            sum.appendChild(t);
            var t = document.createTextNode(all_list.get(i).name);

            content.appendChild(t);

            det.appendChild(sum);
            det.appendChild(content);

            results.appendChild(det);
        }

        viewer.appendChild(results);
    }

    Module.onRuntimeInitialized = start;
})();
