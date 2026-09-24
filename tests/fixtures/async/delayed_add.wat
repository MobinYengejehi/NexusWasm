(module
    (import
        "host"
        "delayed_add"

        (func $delayed_add
            (param i32 i32)
            (result i32)
        )
    )

    (func
        (export "run")
        (result i32)

        i32.const 20
        i32.const 22

        call $delayed_add
    )
)
