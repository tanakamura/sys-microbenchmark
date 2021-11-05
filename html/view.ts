"use strict";

interface EMVector<T> {
    get(i: number): T;
    size() : number;
    delete():any;
}

interface ResultListSet {
    get_sysinfo() : any;
    get_lists(): EMVector<any>
    delete():any;
};

let Chart:any;

class ModuleClass {
    global!:any;
    all_list!:EMVector<any>;
    current_data!:ResultListSet;
    current_lists!:EMVector<any>;
    colors!:string[];
    checkboxes!:HTMLInputElement[];

    GlobalState!:any;
    ResultListSet!:any;
    deserialize_result!:any;
    get_all_benchmark_list!:any;
    vector_Result!:any;

    constructor() {
    }

    hsv_to_rgb(h:number,s:number,v:number):[number, number, number] {
        let h_i = Math.floor(h*6);
        let f = h*6 - h_i;
        let p = v * (1 - s);
        let q = v * (1 - f*s);
        let t = v * (1 - (1 - f) * s);
        let ret;

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

    gen_colors() {
        let nelem = this.current_data.get_lists().size();
        this.colors = new Array<string>(nelem);

        for (let i=0; i<nelem; i++) {
            let h = (1.0/nelem) * i;
            let s = 0.6;
            let v = 1.0;

            let rgb = this.hsv_to_rgb(h,s,v);

            this.colors[i] = "rgb("+rgb[0]+","+rgb[1]+","+rgb[2]+")";
        }
    }

    onRuntimeInitialized () {
        fetch('./combined.json').then(response => response.text()).then(data=>{
            console.log("xx");

            this.global = new this.GlobalState();
            this.all_list = this.get_all_benchmark_list();

            if (this.current_data) {
                this.current_data.delete();
            }

            this.current_data = new this.ResultListSet();

            Module.deserialize_result(this.current_data, data);
            this.current_lists = this.current_data.get_lists();
            this.gen_colors();
            this.update_html(0, null);
        });
    }

    insert_text_p(elem:HTMLElement, text:string) {
        let t = document.createTextNode(text);
        let p = document.createElement('p');
        p.appendChild(t);
        elem.appendChild(p);
    }

    insert_text(elem:HTMLElement, text:string) {
        let t = document.createTextNode(text);
        elem.appendChild(t);
    }

    get_base_index():number {
        let r = document.getElementsByName('base_cpu');
        for (let i=0; i<r.length; i++) {
            var input = r[i] as HTMLInputElement;
            if (input.checked) {
                return i;
            }
        }
        return 0;
    }

    gather_checkbox(): boolean [] {
        let ret = new Array<boolean>(this.checkboxes.length);

        for (let bi=0; bi<this.checkboxes.length; bi++) {
            if (this.checkboxes[bi].checked) {
                ret[bi] = true;
            } else {
                ret[bi] = false;
            }
        }

        return ret;
    }

    update_html(base_index:number, selects:boolean[]|null) {
        let viewer_base = document.getElementById('viewer');
        let child = viewer_base?.lastElementChild;
        while (child) {
            viewer_base?.removeChild(child);
            child = viewer_base?.lastElementChild;
        }

        let viewer = document.createElement('div');
        viewer_base?.appendChild(viewer);

        {
            let machines = document.createElement('div');
            machines.setAttribute('id','machines');

            this.insert_text_p(machines, 'select base target');
            let list = document.createElement('ul');

            let inputs = new Array<HTMLInputElement>();

            for (let i=0; i<this.current_lists.size(); i++) {
                let li = document.createElement('li');
                let cb = document.createElement('input');
                cb.setAttribute('type','radio');
                cb.setAttribute('name', 'base_cpu');
                cb.setAttribute('value', String(i));
                if(i==base_index) {
                    cb.setAttribute('checked','checked');
                }
                inputs.push(cb);

                li.style.backgroundColor = this.colors[i];
                li.appendChild(cb);
                this.insert_text(li, this.current_lists.get(i).get_sysinfo().cpuid + '/' + this.current_lists.get(i).get_sysinfo().os);

                list.appendChild(li);
            }

            machines.appendChild(list);

            this.checkboxes = new Array<HTMLInputElement>(this.current_lists.size());
            this.insert_text_p(machines, 'select compare targets');
            list = document.createElement('ul');
            for (let i=0; i<this.current_lists.size(); i++) {
                let li = document.createElement('li');
                let cb = document.createElement('input');
                cb.setAttribute('type','checkbox');
                cb.setAttribute('name', 'target_cpu'+i);

                if (selects) {
                    if (selects[i]) {
                        cb.setAttribute('checked','checked');
                    }
                } else {
                    cb.setAttribute('checked','checked');
                }

                li.style.backgroundColor = this.colors[i];
                li.appendChild(cb);
                this.checkboxes[i] = cb;
                this.insert_text(li, this.current_lists.get(i).get_sysinfo().cpuid + '/' + this.current_lists.get(i).get_sysinfo().os);

                list.appendChild(li);
            }

            for (let i in inputs) {
                let thisval = this;
                inputs[i].addEventListener("change", function() {
                    thisval.update_html(thisval.get_base_index(),
                                        thisval.gather_checkbox());
                });
            }

            for (let i in this.checkboxes) {
                let thisval = this;
                this.checkboxes[i].addEventListener("change", function() {
                    thisval.update_html(thisval.get_base_index(),
                                        thisval.gather_checkbox());
                });
            }

            machines.appendChild(list);
            viewer.appendChild(machines);
        }

        let results = document.createElement('div');
        results.id = 'results'

        for (let i=0; i<this.all_list.size(); i++) {
            let desc = this.all_list.get(i);
            let det = document.createElement('details');
            let sum = document.createElement('summary');
            let content = document.createElement('div');

            let name = desc.name;
            let prec = desc.get_double_precision();
            let t = document.createTextNode(name);
            sum.appendChild(t);
            t = document.createTextNode(name);
            content.appendChild(t);

            det.appendChild(sum);

            let columns = document.createElement('div');
            let box = document.createElement('div');
            box.appendChild(columns);

            let result_vec = new Module.vector_Result();
            let valid_label_vec = new Array<string>();

            columns.style.display = 'flex';
            let actual_base : number | null = null;
            let actual_index_vec = new Array<number>();
            for (let li=0; li<this.current_lists.size(); li++) {
                if (selects && selects[li] == false) {
                    continue;
                }

                let rl = this.current_lists.get(li);
                let sysinfo = rl.get_sysinfo();
                let rlr = rl.get_results();
                let r = rlr.get(name);

                if (r) {
                    let pre = document.createElement('pre');
                    pre.style.margin = '10pt';
                    pre.style.padding = '5pt';
                    pre.style.fontSize = '10pt';
                    pre.style.backgroundColor = this.colors[li];
                    let contents = sysinfo.cpuid + "\n" + sysinfo.os + "\n";
                    contents += r.get_human_readable(prec);
                    result_vec.push_back(r);
                    r.delete();
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
                let comp = desc.compare(result_vec, actual_base);
                if (comp) {
                    for (let cri=0; cri<comp.size(); cri++) {
                        let cd = comp.get(cri);
                        let datas = cd.get_data();
                        let labels = new Array<string>();
                        let datasets = new Array<any>(result_vec.size());
                        let datasets_data = new Array< Array<number> >(result_vec.size());

                        console.log('name:'+name+','+datas.size()+',',result_vec.size()+',base='+actual_base);

                        for (let dsi=0; dsi<datasets.length; dsi++) {
                            datasets_data[dsi] = new Array<number>(datas.size());
                        }

                        for (let di=0; di<datas.size(); di++) {
                            let data = datas.get(di);
                            let values = data.get_data();
                            labels.push(data.get_str_xlabel());

                            for (let vi=0; vi<values.size(); vi++) {
                                let v = values.get(vi);
                                if (v == -1) {
                                    v = 0;
                                }
                                datasets_data[vi][di] = v;
                            }

                            data.delete();
                        }

                        cd.delete();

                        for (let dsi=0; dsi<datasets.length; dsi++) {
                            datasets[dsi] = {};
                            datasets[dsi].label = valid_label_vec[dsi];
                            datasets[dsi].borderColor = 'red';
                            datasets[dsi].backgroundColor = this.colors[actual_index_vec[dsi]];
                            datasets[dsi].data = datasets_data[dsi];
                        }

                        let data = {
                            labels: labels,
                            datasets: datasets,
                        };

                        const config = {
                            type: 'bar',
                            data: data,
                            options: {
                                responsive: true,
                                plugins: {
                                    legend: {
                                        position: 'top',
                                    },
                                    title: {
                                        display: true,
                                        text: name
                                    }
                                }
                            },
                        };

                        let div = document.createElement('div');

                        div.style.maxWidth = '1024px';
                        div.style.maxHeight = '40%';
                        let canvas = document.createElement('canvas');
                        canvas.style.width = '800px';
                        canvas.style.height = '400px';

                        new Chart(canvas, config);

                        div.appendChild(canvas);
                        box.appendChild(div);
                    }

                    comp.delete();
                }
            }

            result_vec.delete();

            det.appendChild(box);
            results.appendChild(det);

            desc.delete();
        }

        viewer.appendChild(results);
    }
};

let Module = new ModuleClass();
