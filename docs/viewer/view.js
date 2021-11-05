"use strict";
;
var Chart;
var ModuleClass = /** @class */ (function () {
    function ModuleClass() {
    }
    ModuleClass.prototype.hsv_to_rgb = function (h, s, v) {
        var h_i = Math.floor(h * 6);
        var f = h * 6 - h_i;
        var p = v * (1 - s);
        var q = v * (1 - f * s);
        var t = v * (1 - (1 - f) * s);
        var ret;
        switch (h_i) {
            case 0: {
                ret = [v, t, p];
                break;
            }
            case 1: {
                ret = [q, v, p];
                break;
            }
            case 2: {
                ret = [p, v, t];
                break;
            }
            case 3: {
                ret = [p, q, v];
                break;
            }
            case 4: {
                ret = [t, p, v];
                break;
            }
            default: {
                ret = [v, p, q];
                break;
            }
        }
        return [Math.floor(ret[0] * 256),
            Math.floor(ret[1] * 256),
            Math.floor(ret[2] * 256)];
    };
    ModuleClass.prototype.gen_colors = function () {
        var nelem = this.current_data.get_lists().size();
        this.colors = new Array(nelem);
        for (var i = 0; i < nelem; i++) {
            var h = (1.0 / nelem) * i;
            var s = 0.6;
            var v = 1.0;
            var rgb = this.hsv_to_rgb(h, s, v);
            this.colors[i] = "rgb(" + rgb[0] + "," + rgb[1] + "," + rgb[2] + ")";
        }
    };
    ModuleClass.prototype.onRuntimeInitialized = function () {
        var _this = this;
        fetch('./combined.json').then(function (response) { return response.text(); }).then(function (data) {
            _this.global = new _this.GlobalState();
            _this.all_list = _this.get_all_benchmark_list();
            if (_this.current_data) {
                _this.current_data["delete"]();
            }
            _this.current_data = new _this.ResultListSet();
            Module.deserialize_result(_this.current_data, data);
            _this.current_lists = _this.current_data.get_lists();
            _this.gen_colors();
            _this.update_html(0, null);
        });
    };
    ModuleClass.prototype.insert_text_p = function (elem, text) {
        var t = document.createTextNode(text);
        var p = document.createElement('p');
        p.appendChild(t);
        elem.appendChild(p);
    };
    ModuleClass.prototype.insert_text = function (elem, text) {
        var t = document.createTextNode(text);
        elem.appendChild(t);
    };
    ModuleClass.prototype.get_base_index = function () {
        var r = document.getElementsByName('base_cpu');
        for (var i = 0; i < r.length; i++) {
            var input = r[i];
            if (input.checked) {
                return i;
            }
        }
        return 0;
    };
    ModuleClass.prototype.gather_checkbox = function () {
        var ret = new Array(this.checkboxes.length);
        for (var bi = 0; bi < this.checkboxes.length; bi++) {
            if (this.checkboxes[bi].checked) {
                ret[bi] = true;
            }
            else {
                ret[bi] = false;
            }
        }
        return ret;
    };
    ModuleClass.prototype.update_html = function (base_index, selects) {
        var viewer_base = document.getElementById('viewer');
        var child = viewer_base === null || viewer_base === void 0 ? void 0 : viewer_base.lastElementChild;
        while (child) {
            viewer_base === null || viewer_base === void 0 ? void 0 : viewer_base.removeChild(child);
            child = viewer_base === null || viewer_base === void 0 ? void 0 : viewer_base.lastElementChild;
        }
        var viewer = document.createElement('div');
        viewer_base === null || viewer_base === void 0 ? void 0 : viewer_base.appendChild(viewer);
        {
            var machines = document.createElement('div');
            machines.setAttribute('id', 'machines');
            this.insert_text_p(machines, 'select base target');
            var list = document.createElement('ul');
            var inputs = new Array();
            for (var i = 0; i < this.current_lists.size(); i++) {
                var li = document.createElement('li');
                var cb = document.createElement('input');
                cb.setAttribute('type', 'radio');
                cb.setAttribute('name', 'base_cpu');
                cb.setAttribute('value', String(i));
                if (i == base_index) {
                    cb.setAttribute('checked', 'checked');
                }
                inputs.push(cb);
                li.style.backgroundColor = this.colors[i];
                li.appendChild(cb);
                this.insert_text(li, this.current_lists.get(i).get_sysinfo().cpuid + '/' + this.current_lists.get(i).get_sysinfo().os);
                list.appendChild(li);
            }
            machines.appendChild(list);
            this.checkboxes = new Array(this.current_lists.size());
            this.insert_text_p(machines, 'select compare targets');
            list = document.createElement('ul');
            for (var i = 0; i < this.current_lists.size(); i++) {
                var li = document.createElement('li');
                var cb = document.createElement('input');
                cb.setAttribute('type', 'checkbox');
                cb.setAttribute('name', 'target_cpu' + i);
                if (selects) {
                    if (selects[i]) {
                        cb.setAttribute('checked', 'checked');
                    }
                }
                else {
                    cb.setAttribute('checked', 'checked');
                }
                li.style.backgroundColor = this.colors[i];
                li.appendChild(cb);
                this.checkboxes[i] = cb;
                this.insert_text(li, this.current_lists.get(i).get_sysinfo().cpuid + '/' + this.current_lists.get(i).get_sysinfo().os);
                list.appendChild(li);
            }
            var _loop_1 = function (i) {
                var thisval = this_1;
                inputs[i].addEventListener("change", function () {
                    thisval.update_html(thisval.get_base_index(), thisval.gather_checkbox());
                });
            };
            var this_1 = this;
            for (var i in inputs) {
                _loop_1(i);
            }
            var _loop_2 = function (i) {
                var thisval = this_2;
                this_2.checkboxes[i].addEventListener("change", function () {
                    thisval.update_html(thisval.get_base_index(), thisval.gather_checkbox());
                });
            };
            var this_2 = this;
            for (var i in this.checkboxes) {
                _loop_2(i);
            }
            machines.appendChild(list);
            viewer.appendChild(machines);
        }
        var results = document.createElement('div');
        results.id = 'results';
        for (var i = 0; i < this.all_list.size(); i++) {
            var desc = this.all_list.get(i);
            var det = document.createElement('details');
            var sum = document.createElement('summary');
            var content = document.createElement('div');
            var name_1 = desc.name;
            var prec = desc.get_double_precision();
            var t = document.createTextNode(name_1);
            sum.appendChild(t);
            t = document.createTextNode(name_1);
            content.appendChild(t);
            det.appendChild(sum);
            var columns = document.createElement('div');
            var box = document.createElement('div');
            box.appendChild(columns);
            var result_vec = new Module.vector_Result();
            var valid_label_vec = new Array();
            columns.style.display = 'flex';
            var actual_base = null;
            var actual_index_vec = new Array();
            for (var li = 0; li < this.current_lists.size(); li++) {
                if (selects && selects[li] == false) {
                    continue;
                }
                var rl = this.current_lists.get(li);
                var sysinfo = rl.get_sysinfo();
                var rlr = rl.get_results();
                var r = rlr.get(name_1);
                if (r) {
                    var pre = document.createElement('pre');
                    pre.style.margin = '10pt';
                    pre.style.padding = '5pt';
                    pre.style.fontSize = '10pt';
                    pre.style.backgroundColor = this.colors[li];
                    var contents = sysinfo.cpuid + "\n" + sysinfo.os + "\n";
                    contents += r.get_human_readable(prec);
                    result_vec.push_back(r);
                    r["delete"]();
                    this.insert_text(pre, contents);
                    columns.appendChild(pre);
                    valid_label_vec.push(sysinfo.cpuid);
                    actual_index_vec.push(li);
                    if (li == base_index) {
                        actual_base = result_vec.size() - 1;
                    }
                }
            }
            if (actual_base != null) {
                var comp = desc.compare(result_vec, actual_base);
                if (comp) {
                    for (var cri = 0; cri < comp.size(); cri++) {
                        var cd = comp.get(cri);
                        var datas = cd.get_data();
                        var labels = new Array();
                        var datasets = new Array(result_vec.size());
                        var datasets_data = new Array(result_vec.size());
                        for (var dsi = 0; dsi < datasets.length; dsi++) {
                            datasets_data[dsi] = new Array(datas.size());
                        }
                        for (var di = 0; di < datas.size(); di++) {
                            var data_1 = datas.get(di);
                            var values = data_1.get_data();
                            labels.push(data_1.get_str_xlabel());
                            for (var vi = 0; vi < values.size(); vi++) {
                                var v = values.get(vi);
                                if (v == -1) {
                                    v = 0;
                                }
                                datasets_data[vi][di] = v;
                            }
                            data_1["delete"]();
                        }
                        cd["delete"]();
                        for (var dsi = 0; dsi < datasets.length; dsi++) {
                            datasets[dsi] = {};
                            datasets[dsi].label = valid_label_vec[dsi];
                            datasets[dsi].borderColor = 'red';
                            datasets[dsi].backgroundColor = this.colors[actual_index_vec[dsi]];
                            datasets[dsi].data = datasets_data[dsi];
                        }
                        var data = {
                            labels: labels,
                            datasets: datasets
                        };
                        var config = {
                            type: 'bar',
                            data: data,
                            options: {
                                responsive: true,
                                plugins: {
                                    legend: {
                                        position: 'top'
                                    },
                                    title: {
                                        display: true,
                                        text: name_1
                                    }
                                }
                            }
                        };
                        var div = document.createElement('div');
                        div.style.maxWidth = '1024px';
                        div.style.maxHeight = '40%';
                        var canvas = document.createElement('canvas');
                        canvas.style.width = '800px';
                        canvas.style.height = '400px';
                        new Chart(canvas, config);
                        div.appendChild(canvas);
                        box.appendChild(div);
                    }
                    comp["delete"]();
                }
            }
            result_vec["delete"]();
            det.appendChild(box);
            results.appendChild(det);
            desc["delete"]();
        }
        viewer.appendChild(results);
    };
    return ModuleClass;
}());
;
var Module = new ModuleClass();
