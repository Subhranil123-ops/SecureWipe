import Card from "../ui/Card";

function StatCard({
    title,
    value,
}) {
    return (
        <Card>
            <p className="text-sm text-slate-500">
                {title}
            </p>

            <p className="mt-2 text-2xl font-semibold text-slate-900">
                {value}
            </p>
        </Card>
    );
}

export default StatCard;