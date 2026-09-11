(module
    (import
        "does_not_exist"
        "value"
        (func $value
            (result i32)
        )
    )

    (func
        (export "run")
        (result i32)

        call $value
    )
)
