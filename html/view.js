var Module = {
};

Module.onRuntimeInitialized = ()=> {
    var g = new Module.GlobalState();
    var list = Module.get_all_benchmark_list();
    console.log(list);
    for (var i =0; i<list.size(); i++) {
        console.log(list.get(i).name)
    }

    list.delete();
    g.delete();
}
