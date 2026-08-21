function Loading({ message = "Loading..." }) {
    return (
        <div className="flex min-h-[200px] items-center justify-center">
            <p className="text-sm text-slate-500">
                {message}
            </p>
        </div>
    );
}

export default Loading;